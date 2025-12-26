#include "GenericEXT4.h"

/*
 * Reading the Bitmap Block is pretty much
 * the same as Reading an Inode Bitmap.
 * 
 * First you need to get a page, map it and
 * memset it. Then you can get the Bitmap
 * Block addr and convert it to an LBA to
 * read.
*/
uint8_t* GenericEXT4Device::ReadBitmapBlock(BlockGroupDescriptor* GroupDesc) {
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t BitmapBlock = ((uint64_t)GroupDesc->HighAddrBlockBitmap << 32) | GroupDesc->LowAddrBlockBitmap;
    uint64_t BitmapLBA = BitmapBlock * sectorsPerBlock;

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->HighAddrInodeBitmap << 32) | GroupDesc->LowAddrInodeBitmap;

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(BitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to read block bitmap");
            return nullptr;
        }
    }

    uint8_t* Bitmap = (uint8_t*)bufVirt;

    return Bitmap;
}

/*
 * Writing a Bitmap Block is basically just
 * reading the Bitmap Block and then copying
 * the bitmap to the buffer and writing to it.
*/
void GenericEXT4Device::WriteBitmapBlock(BlockGroupDescriptor* GroupDesc, uint8_t* bitmap) {
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t BitmapBlock = ((uint64_t)GroupDesc->HighAddrBlockBitmap << 32) | GroupDesc->LowAddrBlockBitmap;
    uint64_t BitmapLBA = BitmapBlock * sectorsPerBlock;

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->HighAddrInodeBitmap << 32) | GroupDesc->LowAddrInodeBitmap;

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(BitmapLBA + i, (uint8_t*)bufVirt + i * pdev->SectorSize())) {
            _ds->Println("Failed to read block bitmap");
            return;
        }
    }

    memcpy((void*)bufVirt, bitmap, blockSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->WriteSector(BitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to write block bitmap");
            return;
        }
    }

    _ds->UnMapMemory((void*)bufVirt);
    _ds->FreePage(bufPhys);
}

/*
 * To update a block bitmap checksum,
 * you need to use the CheckUUID as a
 * seed and crc32c the whole bitmap
*/
void GenericEXT4Device::UpdateBlockBitmapChksum(uint32_t group, BlockGroupDescriptor* GroupDesc) {
    if (superblock->MetaCheckAlgo == 1) {
        uint64_t blockSize = 1024ull << superblock->BlockSize;

        uint8_t* bitmap = ReadBitmapBlock(GroupDesc);

        uint32_t crc = crc32c_sw(superblock->CheckUUID, bitmap, blockSize);

        GroupDesc->LowChkBlockBitmap = crc & 0xFFFF;
        if (superblock->RequiredFeatures & BITS64) {
            _ds->Println("64 Bit Bitmap Checksum");
            GroupDesc->HighChkBlockBitmap = (crc >> 16);
        }
    }
}

/*
 * To Allocate a Block we must first find a free Block.
 * To do this, we can iterate through all the blocks
 * and check if it has free blocks.
 * If it does, then we can mark it as used and use it.
*/
uint32_t GenericEXT4Device::AllocateBlock(FsNode* parent) {
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    /*
     * First we must find the Parent's Block Group
    */
    uint32_t parentBlockGroup = (parent->nodeId - 1) / superblock->InodesPerBlockGroup;

    if (parentBlockGroup < superblock->FirstDataBlock) {
        parentBlockGroup = superblock->FirstDataBlock;
    }

    BlockGroupDescriptor* GroupDesc = GroupDescs[parentBlockGroup];
    uint64_t inodeTableStart = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | GroupDesc->LowAddrInodeTable;
    uint64_t inodeTableBlocks = (superblock->InodesPerBlockGroup * superblock->InodeSize + blockSize - 1) / blockSize;

    uint32_t firstUsableBlock = inodeTableStart + inodeTableBlocks;

    /*
     * Get some Flex Stuff
    */
    uint32_t flexSize = 1u << superblock->GroupsPerFlex;
    uint32_t firstFlexGroup = parentBlockGroup - (parentBlockGroup % flexSize);
    uint32_t endFlexGroup = firstFlexGroup + flexSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint32_t BlockGroup = firstFlexGroup; BlockGroup < endFlexGroup; BlockGroup++) {
        BlockGroupDescriptor* GroupDesc = GroupDescs[BlockGroup];

        uint32_t freeBlocks = ((uint32_t)GroupDesc->HighUnallocBlocks << 16) | GroupDesc->LowUnallocBlocks;

        uint64_t inodeTableStart = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | GroupDesc->LowAddrInodeTable;
        uint64_t inodeTableBlocks = (superblock->InodesPerBlockGroup * superblock->InodeSize + blockSize - 1) / blockSize;

        if (freeBlocks == 0) continue;

        /*
         * Let's read our Block Bitmap
        */
        uint8_t* Bitmap = ReadBitmapBlock(GroupDesc);

        uint32_t BlocksPerGroup = superblock->BlocksPerBlockGroup;

        uint32_t maxBits = blockSize * 8;
        uint32_t limit = (BlocksPerGroup < maxBits) ? BlocksPerGroup : maxBits;

        for (uint32_t i = 0; i < limit; i++) {
            uint64_t GlobalBlock = (uint64_t)BlockGroup * BlocksPerGroup + i;
            uint64_t GroupStart = (uint64_t)BlockGroup * BlocksPerGroup;

            if (GlobalBlock == 0) continue;
            if (GlobalBlock < firstUsableBlock) continue;

            /*
             * Now we can check if it is in use
            */
            uint32_t byte_index = i / 8;
            uint8_t bit_index = i % 8;

            if (Bitmap[byte_index] & (1 << bit_index)) {
                continue;
            }

            Bitmap[byte_index] |= (1 << bit_index);

            WriteBitmapBlock(GroupDesc, Bitmap);

            superblock->UnallocBlocks--;

            uint32_t count = (GroupDesc->HighUnallocBlocks << 16) | GroupDesc->LowUnallocBlocks;
            count--;
            GroupDesc->LowUnallocBlocks = count & 0xFFFF;
            GroupDesc->HighUnallocBlocks = count >> 16;

            UpdateSuperblock();
            UpdateGroupDesc(BlockGroup, GroupDesc);

            return GlobalBlock;
        }
    }
    return 0;
}