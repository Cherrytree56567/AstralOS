#include "../GenericEXT4.h"

/*
 * So a Bitmap is basically data that tells
 * you which part of data of that type is free.
 * 
 * For Example, the Inode Bitmap tells you which
 * Inodes are free.
 * 
 * So to read a Bitmap Inode you must first get
 * a page and then map and memset it.
 * 
 * Then you must get the blockSize and sectors
 * Per Block.
 * 
 * We can then get the Inode Bitmap Block so that
 * we can get the LBA. To Read the Bitmap Block,
 * you must convert the block to an LBA.
 * 
 * Then we can read and return our Bitmap.
*/
uint8_t* GenericEXT4Device::ReadBitmapInode(BlockGroupDescriptor* GroupDesc) {
    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->bg_inode_bitmap_hi << 32) | GroupDesc->bg_inode_bitmap_lo;
    uint64_t inodeBitmapLBA = inodeBitmapBlock * sectorsPerBlock;

    /*
     * Now we can read our Inode Bitmap
    */
    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(inodeBitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to read inode bitmap");
            return 0;
        }
    }

    return (uint8_t*)bufVirt;
}

/*
 * Writing a Bitmap Inode is pretty much the same as
 * Reading a Bitmap Inode but this time we need to 
 * copy the bitmap to our in func buffer so that we
 * can write to the Bitmap.
 * 
 * Next, we can just Unmap and free the page.
*/
void GenericEXT4Device::WriteBitmapInode(BlockGroupDescriptor* GroupDesc, uint8_t* bitmap) {
    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->bg_inode_bitmap_hi << 32) | GroupDesc->bg_inode_bitmap_lo;
    uint64_t inodeBitmapLBA = inodeBitmapBlock * sectorsPerBlock;

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(inodeBitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to read inode bitmap");
            return;
        }
    }
    
    memcpy((void*)bufVirt, bitmap, blockSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->WriteSector(inodeBitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to write inode bitmap");
            return;
        }
    }

    _ds->UnMapMemory((void*)bufVirt);
    _ds->FreePage(bufPhys);
}

/*
 * To Allocate an Inode we need to 
 * first look through all block
 * group descriptors and check if 
 * they have a free Inode. 
 * 
 * Then we can get and Inode Bitmap 
 * Block and size.
 * 
 * Then we can just read the Inode
 * Bitmap Block and mark it used
 * and write to the bitmap.
 * 
 * Then we must Decrement UnallocInodes
 * from the superblock and the Low
 * and High UnallocInodes from the
 * Group Descriptor.
*/
uint32_t GenericEXT4Device::AllocateInode(FsNode* parent) {
    /*
     * First we need to calculate the Parent's Block Group and Index
     * so that we can get our Group Descriptor.
     * 
     * Then we can get our FlexSize and FirstFlex bc we are using Flex
     * Block Groups
    */
    uint32_t ParentBlockGroup = (parent->nodeId - 1) / superblock->s_inodes_per_group;

    uint32_t FlexSize = 1u << superblock->s_log_groups_per_flex;
    uint32_t FirstFlex = ParentBlockGroup - (ParentBlockGroup % FlexSize);

    uint64_t totalBlocks = ((uint64_t)superblock->s_blocks_count_hi << 32) | superblock->s_blocks_count_lo;
    uint32_t groupCount = (totalBlocks + superblock->s_blocks_per_group - 1) / superblock->s_blocks_per_group;

    uint32_t flexEnd = FirstFlex + FlexSize;
    if (flexEnd > groupCount) {
        flexEnd = groupCount;
    }

    for (int i = FirstFlex; i < flexEnd; i++) {
        if (i == 0) continue;
        BlockGroupDescriptor* GroupDesc = GroupDescs[i];

        /*
        * Then we can allocate a page to use for the
        * rest of our func.
        * 
        * To re-use it, we can just memset it again.
        */
        uint8_t* bitmap = ReadBitmapInode(GroupDesc);

        uint32_t FlexInodeOff = (ParentBlockGroup - i) * superblock->s_inodes_per_group;
        uint32_t First = (i == 0) ? (superblock->s_first_ino - 1) : 0;
        uint32_t newInode = 0xFFFFFFFF;

        /*
        * Now we can scan for a Free Inode
        * using our Inode Bitmap.
        */
        
        for (uint32_t s = First; s < superblock->s_inodes_per_group; s++) {
            uint32_t byte = s >> 3;
            uint8_t bit = s & 7;

            if (!(bitmap[byte] & (1 << bit))) {
                bitmap[byte] |= (1 << bit);
                newInode = s;
                break;
            }
        }

        if (newInode == 0xFFFFFFFF) {
            _ds->Println("No free inode");
            continue;
        }

        WriteBitmapInode(GroupDesc, bitmap);
        
        superblock->s_free_inodes_count--;
        UpdateSuperblock();
        uint32_t UnallocInodes = ((uint32_t)GroupDesc->bg_free_inodes_count_hi << 16) | GroupDesc->bg_free_inodes_count_lo;

        UnallocInodes--;
        GroupDesc->bg_free_inodes_count_lo = UnallocInodes & 0xFFFF;
        GroupDesc->bg_free_inodes_count_hi = UnallocInodes >> 16;

        UpdateGroupDesc(i, GroupDesc);

        uint32_t globalInode = i * superblock->s_inodes_per_group + newInode + 1;
        return globalInode;
    }

    return NULL;
}

/*
 * If we enable Checksums, which is almost
 * always enabled on all EXT4 Filesystems, 
 * then we must update the checksum when
 * we change the Inode Bitmap.
 * 
 * To do this, we need a crc32c checksum
 * of the bitmap using the CheckUUID as the
 * seed.
*/
void GenericEXT4Device::UpdateInodeBitmapChksum(uint32_t group, BlockGroupDescriptor* GroupDesc) {
    if (superblock->s_checksum_type == 1) {
        uint64_t blockSize = 1024ull << superblock->s_log_block_size;

        uint8_t* bitmap = ReadBitmapInode(GroupDesc);

        uint32_t crc = crc32c_sw(~0, superblock->s_uuid, 16);
        crc = crc32c_sw(crc, bitmap, blockSize);

        GroupDesc->bg_inode_bitmap_csum_lo = crc & 0xFFFF;
        if (superblock->s_feature_incompat & IncompatFeatures::INCOMPAT_64BIT) {
            _ds->Println("64 Bit Bitmap Checksum");
            GroupDesc->bg_inode_bitmap_csum_hi = (crc >> 16);
        }
    }
}

/*
 * To Read an Inode we must first get the 
 * Inode Block Group and Index.
 * 
 * Then we can request a page and map and 
 * memset it.
 * 
 * Then we can get our Group Descriptor to
 * figure out where our Inode is so that
 * we can read it.
*/
Inode* GenericEXT4Device::ReadInode(uint64_t node) {
    uint64_t InodeBlockGroup = (node - 1) / superblock->s_inodes_per_group;
    uint64_t InodeIndex = (node - 1) % superblock->s_inodes_per_group;

    uint64_t BlockSize = 1024ull << superblock->s_log_block_size;
    uint64_t SectorSize = pdev->SectorSize();
    uint64_t SectorsPerBlock = BlockSize / SectorSize;

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    BlockGroupDescriptor* GroupDesc = GroupDescs[InodeBlockGroup];

    uint64_t InodeTableBlock = ((uint64_t)GroupDesc->bg_inode_table_hi << 32) | (uint64_t)GroupDesc->bg_inode_table_lo;
    uint64_t InodeTableLBA = InodeTableBlock * SectorsPerBlock;
    uint64_t InodeOffset = InodeIndex * superblock->s_inode_size;
    uint64_t InodeSize = superblock->s_inode_size;

    uint64_t startSectorIndex = InodeOffset / SectorSize;
    uint64_t sectorOffset = InodeOffset % SectorSize;

    uint64_t bytesNeeded = sectorOffset + InodeSize;
    uint64_t sectorsToRead = (bytesNeeded + SectorSize - 1) / SectorSize;
    for (uint64_t i = 0; i < sectorsToRead; i++) {
        uint64_t LBA = InodeTableLBA + startSectorIndex + i;
        if (!pdev->ReadSector(LBA, (void*)(bufPhys + i * SectorSize))) {
            _ds->Println("Failed to read root inode");
            return nullptr;
        }
    }
    return (Inode*)(bufVirt + sectorOffset);
}

/*
 * Before, the CreateDir Func was too cluttered,
 * so now, Im making it less cluttered.
 * 
 * This one is pretty much the exact same as read
 * Inode but we just copy the Inode to our buffer
 * and then write it.
 * 
 * The memset `tmp` stuff was from GPT bc I was
 * having trouble writng in the Inode.
*/
void GenericEXT4Device::WriteInode(uint32_t inodeNum, Inode* ind) {
    if (superblock->s_checksum_type == 1) {
        ind->i_checksum_hi = 0;
        if (superblock->s_creator_os == CreatorOSIDs::Linux) {
            ind->i_osd2.linux2.l_i_checksum_lo = 0;
        } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
            ind->i_osd2.astral2.a_i_checksum_lo = 0;
        }
        uint32_t crc = crc32c_sw(~0, superblock->s_uuid, 16);
        crc = crc32c_sw(crc, (unsigned char *)&inodeNum, sizeof(inodeNum));
        crc = crc32c_sw(crc, (unsigned char *)&ind->i_generation, sizeof(ind->i_generation));
        uint32_t size = 128 + ind->i_size_high;
        crc = crc32c_sw(crc, (unsigned char *)ind, size);
        
        if (superblock->s_creator_os == CreatorOSIDs::Linux) {
            ind->i_osd2.linux2.l_i_checksum_lo = crc & 0xFFFF;
        } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
            ind->i_osd2.astral2.a_i_checksum_lo = crc & 0xFFFF;
        }
        ind->i_checksum_hi = (crc >> 16);
    }
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / sectorSize;

    uint32_t inodeIndex = inodeNum - 1;
    uint32_t inodesPerGroup = superblock->s_inodes_per_group;

    uint32_t group = inodeIndex / inodesPerGroup;
    uint32_t indexInGroup = inodeIndex % inodesPerGroup;
    uint64_t inodeSize = superblock->s_inode_size;
    uint64_t byteOffset = indexInGroup * inodeSize;

    BlockGroupDescriptor* GroupDesc = GroupDescs[group];
    if (!GroupDesc) {
        _ds->Println("Group Desc Is Invalid");
        return;
    }
    uint64_t inodeTableBlock = ((uint64_t)GroupDesc->bg_inode_table_hi << 32) | (uint64_t)GroupDesc->bg_inode_table_lo;

    uint64_t blockOffset = byteOffset / blockSize;
    uint64_t offsetInBlock = byteOffset % blockSize;

    uint64_t LBA = (inodeTableBlock + blockOffset) * sectorsPerBlock;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("Failed to Write Inode");
            return;
        }
    }
    
    uint8_t tmp[4096];
    memset(tmp, 0, inodeSize);
    memcpy(tmp, ind, sizeof(Inode));
    memcpy((uint8_t*)bufVirt + offsetInBlock, tmp, inodeSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->WriteSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("Failed to Write Inode [Write Sector]");
            return;
        }
    }
}