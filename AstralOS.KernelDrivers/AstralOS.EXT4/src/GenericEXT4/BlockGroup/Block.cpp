#include "../GenericEXT4.h"

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
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t BitmapBlock = ((uint64_t)GroupDesc->bg_block_bitmap_hi << 32) | GroupDesc->bg_block_bitmap_lo;
    uint64_t BitmapLBA = BitmapBlock * sectorsPerBlock;

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->bg_inode_bitmap_hi << 32) | GroupDesc->bg_inode_bitmap_lo;

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
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t BitmapBlock = ((uint64_t)GroupDesc->bg_block_bitmap_hi << 32) | GroupDesc->bg_block_bitmap_lo;
    uint64_t BitmapLBA = BitmapBlock * sectorsPerBlock;

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->bg_inode_bitmap_hi << 32) | GroupDesc->bg_inode_bitmap_lo;

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
    if (superblock->s_checksum_type == 1) {
        uint64_t blockSize = 1024ull << superblock->s_log_block_size;

        uint8_t* bitmap = ReadBitmapBlock(GroupDesc);

        uint32_t crc = crc32c_sw(~0, superblock->s_uuid, 16);
        crc = crc32c_sw(crc, bitmap, blockSize);

        GroupDesc->bg_block_bitmap_csum_lo = crc & 0xFFFF;
        if (superblock->s_feature_incompat & IncompatFeatures::INCOMPAT_64BIT) {
            _ds->Println("64 Bit Bitmap Checksum");
            GroupDesc->bg_block_bitmap_csum_hi = (crc >> 16);
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
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    /*
     * First we must find the Parent's Block Group
    */
    uint32_t parentBlockGroup = (parent->nodeId - 1) / superblock->s_inodes_per_group;

    if (parentBlockGroup < superblock->s_first_data_block) {
        parentBlockGroup = superblock->s_first_data_block;
    }

    BlockGroupDescriptor* GroupDesc = GroupDescs[parentBlockGroup];
    uint64_t inodeTableStart = ((uint64_t)GroupDesc->bg_inode_table_hi << 32) | GroupDesc->bg_inode_table_lo;
    uint64_t inodeTableBlocks = (superblock->s_inodes_per_group * superblock->s_inode_size + blockSize - 1) / blockSize;

    uint32_t firstUsableBlock = inodeTableStart + inodeTableBlocks;

    /*
     * Get some Flex Stuff
    */
    uint32_t flexSize = 1u << superblock->s_log_groups_per_flex;
    uint32_t firstFlexGroup = parentBlockGroup - (parentBlockGroup % flexSize);
    uint32_t endFlexGroup = firstFlexGroup + flexSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint32_t BlockGroup = firstFlexGroup; BlockGroup < endFlexGroup; BlockGroup++) {
        BlockGroupDescriptor* GroupDesc = GroupDescs[BlockGroup];

        uint32_t freeBlocks = ((uint32_t)GroupDesc->bg_free_blocks_count_hi << 16) | GroupDesc->bg_free_blocks_count_lo;

        uint64_t inodeTableStart = ((uint64_t)GroupDesc->bg_inode_table_hi << 32) | GroupDesc->bg_inode_table_lo;
        uint64_t inodeTableBlocks = (superblock->s_inodes_per_group * superblock->s_inode_size + blockSize - 1) / blockSize;

        if (freeBlocks == 0) continue;

        /*
         * Let's read our Block Bitmap
        */
        uint8_t* Bitmap = ReadBitmapBlock(GroupDesc);

        uint32_t BlocksPerGroup = superblock->s_blocks_per_group;

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

            uint32_t UnallocBlocks = (superblock->s_free_blocks_count_hi << 16) | superblock->s_free_blocks_count_lo;
            UnallocBlocks--;
            superblock->s_free_blocks_count_hi = UnallocBlocks >> 16;
            superblock->s_free_blocks_count_lo = UnallocBlocks & 0xFFFF;

            uint32_t count = (GroupDesc->bg_free_blocks_count_hi << 16) | GroupDesc->bg_free_blocks_count_lo;
            count--;
            GroupDesc->bg_free_blocks_count_lo = count & 0xFFFF;
            GroupDesc->bg_free_blocks_count_hi = count >> 16;

            UpdateSuperblock();
            UpdateGroupDesc(BlockGroup, GroupDesc);

            return GlobalBlock;
        }
    }
    return 0;
}

uint32_t GenericEXT4Device::AllocateBlocks(FsNode* parent, uint64_t& count) {
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    /*
     * First we must find the Parent's Block Group
    */
    uint32_t parentBlockGroup = (parent->nodeId - 1) / superblock->s_inodes_per_group;

    if (parentBlockGroup < superblock->s_first_data_block) {
        parentBlockGroup = superblock->s_first_data_block;
    }

    BlockGroupDescriptor* GroupDesc = GroupDescs[parentBlockGroup];
    uint64_t inodeTableStart = ((uint64_t)GroupDesc->bg_inode_table_hi << 32) | GroupDesc->bg_inode_table_lo;
    uint64_t inodeTableBlocks = (superblock->s_inodes_per_group * superblock->s_inode_size + blockSize - 1) / blockSize;

    uint32_t firstUsableBlock = inodeTableStart + inodeTableBlocks;

    /*
     * Get some Flex Stuff
    */
    uint32_t flexSize = 1u << superblock->s_log_groups_per_flex;
    uint32_t firstFlexGroup = parentBlockGroup - (parentBlockGroup % flexSize);
    uint32_t endFlexGroup = firstFlexGroup + flexSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint32_t run_start = 0;
    uint32_t run_len = 0;

    uint32_t best_start = 0;
    uint32_t best_len = 0;

    for (uint32_t BlockGroup = firstFlexGroup; BlockGroup < endFlexGroup; BlockGroup++) {
        BlockGroupDescriptor* GroupDesc = GroupDescs[BlockGroup];

        uint32_t freeBlocks = ((uint32_t)GroupDesc->bg_free_blocks_count_hi << 16) | GroupDesc->bg_free_blocks_count_lo;

        uint64_t inodeTableStart = ((uint64_t)GroupDesc->bg_inode_table_hi << 32) | GroupDesc->bg_inode_table_lo;
        uint64_t inodeTableBlocks = (superblock->s_inodes_per_group * superblock->s_inode_size + blockSize - 1) / blockSize;

        if (freeBlocks == 0) continue;

        /*
         * Let's read our Block Bitmap
        */
        uint8_t* Bitmap = ReadBitmapBlock(GroupDesc);

        uint32_t BlocksPerGroup = superblock->s_blocks_per_group;

        uint32_t maxBits = blockSize * 8;
        uint32_t limit = (BlocksPerGroup < maxBits) ? BlocksPerGroup : maxBits;

        for (uint32_t i = 0; i < limit; i++) {
            uint64_t GlobalBlock = (uint64_t)BlockGroup * BlocksPerGroup + i;
            uint64_t GroupStart = (uint64_t)BlockGroup * BlocksPerGroup;

            uint64_t BlockBitmap = ((uint64_t)GroupDesc->bg_block_bitmap_hi << 32) | GroupDesc->bg_block_bitmap_lo;
            uint64_t InodeBitmap = ((uint64_t)GroupDesc->bg_inode_bitmap_hi << 32) | GroupDesc->bg_inode_bitmap_lo;
            uint64_t InodeTable = ((uint64_t)GroupDesc->bg_inode_table_hi << 32) | GroupDesc->bg_inode_table_lo;

            if (GlobalBlock == 0) {
                run_len = 0;
                continue;
            }

            if (GlobalBlock == BlockBitmap) {
                run_len = 0;
                continue;
            }
            if (GlobalBlock == InodeBitmap) {
                run_len = 0;
                continue;
            }
            if (GlobalBlock >= InodeTable && GlobalBlock < InodeTable + inodeTableBlocks) {
                run_len = 0;
                continue;
            }

            if (GlobalBlock < firstUsableBlock) {
                run_len = 0;
                continue;
            }

            /*
             * Now we can check if it is in use
            */
            uint32_t byte_index = i / 8;
            uint8_t bit_index = i % 8;

            if ((Bitmap[byte_index] & (1 << bit_index)) == 0) {
                if (run_len == 0) {
                    run_start = i;
                }

                if (run_len == count) {
                    break;
                }

                run_len++;

                if (run_len > best_len) {
                    best_len = run_len;
                    best_start = run_start;
                }
            } else {
                run_len = 0;
            }
        }

        if (best_len > 0) {
            uint32_t alloc_len = best_len;

            uint32_t UnallocBlocks = (superblock->s_free_blocks_count_hi << 16) | superblock->s_free_blocks_count_lo;
            uint32_t GDcount = (GroupDesc->bg_free_blocks_count_hi << 16) | GroupDesc->bg_free_blocks_count_lo;

            for (uint32_t j = 0; j < alloc_len; j++) {
                uint32_t bit = best_start + j;
                Bitmap[bit / 8] |= (1 << (bit % 8));

                UnallocBlocks--;
                GDcount--;
                count--;
            }

            WriteBitmapBlock(GroupDesc, Bitmap);

            superblock->s_free_blocks_count_hi = UnallocBlocks >> 16;
            superblock->s_free_blocks_count_lo = UnallocBlocks & 0xFFFF;
            GroupDesc->bg_free_blocks_count_lo = GDcount & 0xFFFF;
            GroupDesc->bg_free_blocks_count_hi = GDcount >> 16;

            UpdateSuperblock();
            UpdateGroupDesc(BlockGroup, GroupDesc);

            return (uint64_t)BlockGroup * BlocksPerGroup + best_start;
        }
    }
    return 0;
}