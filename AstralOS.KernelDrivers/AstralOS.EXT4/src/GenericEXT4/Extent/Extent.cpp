#include "../GenericEXT4.h"

uint64_t GenericEXT4Device::CountExtents(ExtentHeader* hdr) {
    uint64_t extentsCount = 0;
    if (hdr->eh_depth == 0) {
        for (int i = 0; i < hdr->eh_entries; i++) {
            extentsCount++;
            //Extent* ee = (Extent*)((uint64_t)hdr + sizeof(ExtentHeader) + i * sizeof(Extent));
        }
    } else {
        void* buf = _ds->RequestPage();
        uint64_t bufPhys = (uint64_t)buf;
        uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
        _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

        uint64_t blockSize = 1024ull << superblock->s_log_block_size;
        uint64_t sectorSize = pdev->SectorSize();
        uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

        for (int i = 0; i < hdr->eh_entries; i++) {
            memset((void*)bufVirt, 0, 4096);

            ExtentIDX* ei = (ExtentIDX*)((uint64_t)hdr + sizeof(ExtentHeader) + i * sizeof(ExtentIDX));

            uint64_t block = ((uint64_t)ei->ei_leaf_hi << 32) | (uint64_t)ei->ei_leaf_lo;

            uint64_t LBA = block * sectorsPerBlock;
                    
            for (uint64_t x = 0; x < sectorsPerBlock; x++) {
                if (!pdev->ReadSector(LBA + x, (void*)((uint8_t*)bufPhys + x * pdev->SectorSize()))) {
                    _ds->Println("Failed to read directory block");
                    return 0;
                }
            }

            ExtentHeader* eh = (ExtentHeader*)bufVirt;

            if (eh->eh_magic != 0xF30A) continue;
            if (eh->eh_depth != (hdr->eh_depth - 1)) continue;

            extentsCount += CountExtents(eh);
        }
        _ds->UnMapMemory((void*)bufVirt);
        _ds->FreePage(buf);
    }

    return extentsCount;
}

/*
 * I wonder which would be better, to
 * count all the extents beforehand and 
 * to allocate accordingly or to allocate
 * and copy to the new buffer whenever
 * needed.
*/
Extent** GenericEXT4Device::GetExtents(ExtentHeader* hdr, uint64_t& extentsCount) {
    if (!hdr || hdr->eh_magic != 0xF30A) return nullptr;

    extentsCount = 0;

    uint64_t extentCount = CountExtents(hdr);

    Extent** extents = (Extent**)_ds->malloc(sizeof(Extent*) * extentCount);

    if (hdr->eh_depth == 0) {
        for (int i = 0; i < hdr->eh_entries; i++) {
            Extent* ee = (Extent*)((uint64_t)hdr + sizeof(ExtentHeader) + i * sizeof(Extent));
            Extent* eeCopy = (Extent*)_ds->malloc(sizeof(Extent));

            *eeCopy = *ee;

            extents[extentsCount] = eeCopy;
            extentsCount++;
        }
    } else {
        void* buf = _ds->RequestPage();
        uint64_t bufPhys = (uint64_t)buf;
        uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
        _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

        uint64_t blockSize = 1024ull << superblock->s_log_block_size;
        uint64_t sectorSize = pdev->SectorSize();
        uint64_t sectorsPerBlock = blockSize / sectorSize;

        for (int i = 0; i < hdr->eh_entries; i++) {
            memset((void*)bufVirt, 0, 4096);

            ExtentIDX* ei = (ExtentIDX*)((uint64_t)hdr + sizeof(ExtentHeader) + i * sizeof(ExtentIDX));

            uint64_t block = ((uint64_t)ei->ei_leaf_hi << 32) | (uint64_t)ei->ei_leaf_lo;

            uint64_t LBA = block * sectorsPerBlock;
                    
            for (uint64_t x = 0; x < sectorsPerBlock; x++) {
                if (!pdev->ReadSector(LBA + x, (void*)((uint8_t*)bufPhys + x * pdev->SectorSize()))) {
                    _ds->Println("Failed to read directory block");
                    return 0;
                }
            }

            ExtentHeader* eh = (ExtentHeader*)bufVirt;

            if (eh->eh_magic != 0xF30A) continue;
            if (eh->eh_depth != (hdr->eh_depth - 1)) continue;

            uint64_t extsCount = 0;
            Extent** exts = GetExtents(eh, extsCount);

            for (uint64_t x = 0; x < extsCount; x++) {
                extents[extentsCount] = exts[x];
                extentsCount++;
            }
            _ds->free(exts);
        }
        _ds->UnMapMemory((void*)bufVirt);
        _ds->FreePage(buf);
    }
    return extents;
}

bool GenericEXT4Device::AddExtentDepth(ExtentHeader* hdr, uint64_t hdrBlock, Extent ext) {
    if (!hdr || hdr->eh_magic != 0xF30A) return false;

    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / sectorSize;

    if (hdr->eh_depth == 0) {
        if (hdr->eh_max != hdr->eh_entries) {
            Extent* ee = (Extent*)((uint64_t)hdr + sizeof(ExtentHeader) + hdr->eh_entries * sizeof(Extent));
            *ee = ext;

            hdr->eh_entries++;

            uint64_t LBA = hdrBlock * sectorsPerBlock;

            for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                if (!pdev->WriteSector(LBA + i, (uint8_t*)hdr + i * sectorSize)) {
                    _ds->Println("Failed to flush extent leaf");
                    return false;
                }
            }
            
            return true;
        }
    } else {
        void* buf = _ds->RequestPage();
        uint64_t bufPhys = (uint64_t)buf;
        uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
        _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

        uint64_t blockSize = 1024ull << superblock->s_log_block_size;
        uint64_t sectorSize = pdev->SectorSize();
        uint64_t sectorsPerBlock = blockSize / sectorSize;

        for (int i = 0; i < hdr->eh_entries; i++) {
            memset((void*)bufVirt, 0, 4096);

            ExtentIDX* ei = (ExtentIDX*)((uint64_t)hdr + sizeof(ExtentHeader) + i * sizeof(ExtentIDX));

            uint64_t block = ((uint64_t)ei->ei_leaf_hi << 32) | (uint64_t)ei->ei_leaf_lo;

            uint64_t LBA = block * sectorsPerBlock;
                    
            for (uint64_t x = 0; x < sectorsPerBlock; x++) {
                if (!pdev->ReadSector(LBA + x, (void*)((uint8_t*)bufPhys + x * pdev->SectorSize()))) {
                    _ds->Println("Failed to read directory block");
                    return false;
                }
            }

            ExtentHeader* eh = (ExtentHeader*)bufVirt;

            if (eh->eh_magic != 0xF30A) continue;
            if (eh->eh_depth != (hdr->eh_depth - 1)) continue;

            if (AddExtentDepth(eh, block, ext)) {
                return true;
            }
        }
        _ds->UnMapMemory((void*)bufVirt);
        _ds->FreePage(buf);
    }
    return false;
}

bool GenericEXT4Device::AddExtent(FsNode* fsN, Inode* ind, Extent ext) {
    ExtentHeader* eh = (ExtentHeader*)ind->i_block;

    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    if (eh->eh_magic != 0xF30A) {
        eh->eh_magic = 0xF30A;
        eh->eh_depth = 0;
        eh->eh_entries = 0;
        eh->eh_generation = 0;
        eh->eh_max = (sizeof(ind->i_block) - sizeof(ExtentHeader)) / sizeof(Extent);
    }

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    if (eh->eh_depth == 0) {
        if (eh->eh_entries >= eh->eh_max) {
            uint32_t block = AllocateBlock(fsN);

            /*
             * Create an Extent Header for our new Extent
             * Block and copy the old extents to our block.
            */
            ExtentHeader* newHdr = (ExtentHeader*)bufVirt;
            newHdr->eh_magic = 0xF30A;
            newHdr->eh_depth = 0;
            newHdr->eh_generation = 0;
            newHdr->eh_entries = eh->eh_entries;
            newHdr->eh_max = (blockSize - sizeof(ExtentHeader)) / sizeof(Extent);

            void* src = (void*)((uint8_t*)eh + sizeof(ExtentHeader));
            void* dst = (void*)((uint8_t*)newHdr + sizeof(ExtentHeader));

            memcpy(dst, src, eh->eh_entries * sizeof(Extent));

            /*
             * Add our new Extent
            */
            Extent* ee = (Extent*)((uint8_t*)bufVirt + sizeof(ExtentHeader) + (sizeof(Extent) * eh->eh_entries));
            ee->ee_block = ext.ee_block;
            ee->ee_len = ext.ee_len;
            ee->ee_start_lo = ext.ee_start_lo;
            ee->ee_start_hi = ext.ee_start_hi;

            newHdr->eh_entries++;

            uint64_t blockLBA = block * (blockSize / pdev->SectorSize());
            for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                if (!pdev->WriteSector(blockLBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                    _ds->Println("Failed to write directory block");
                    return false;
                }
            }

            /*
             * Point our inode Extent block to our new Extent Block
            */
            eh->eh_entries = 1;
            eh->eh_depth += 1;

            ExtentIDX* idx = (ExtentIDX*)((uint8_t*)ind->i_block + sizeof(ExtentHeader));

            Extent* lowestee = (Extent*)(eh + sizeof(ExtentHeader));

            idx->ei_block = lowestee->ee_block;
            idx->ei_leaf_lo = (uint32_t)block;
            idx->ei_leaf_hi = (uint16_t)(block >> 32);
            idx->ei_unused = 0;

            memset((uint8_t*)ind->i_block + sizeof(ExtentHeader) + sizeof(ExtentIDX), 0, sizeof(ind->i_block) - (sizeof(ExtentHeader) + sizeof(ExtentIDX)));

            uint64_t blocks = (uint64_t)ind->i_blocks_lo;
            if (superblock->s_creator_os == CreatorOSIDs::Linux) {
                blocks = ((uint64_t)ind->i_osd2.linux2.l_i_blocks_high << 32) | (uint64_t)ind->i_blocks_lo;
            } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
                blocks = ((uint64_t)ind->i_osd2.astral2.a_i_blocks_high << 32) | (uint64_t)ind->i_blocks_lo;
            }
            blocks++;

            ind->i_blocks_lo = (uint32_t)(blocks & 0xFFFFFFFF);

            if (superblock->s_creator_os == CreatorOSIDs::Linux) {
                ind->i_osd2.linux2.l_i_blocks_high = (uint32_t)(blocks >> 32);
            } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
                ind->i_osd2.astral2.a_i_blocks_high = (uint32_t)(blocks >> 32);
            }
            return true;
        } else {
            Extent* ee = (Extent*)((uint8_t*)ind->i_block + sizeof(ExtentHeader) + (sizeof(Extent) * eh->eh_entries));
            ee->ee_block = ext.ee_block;
            ee->ee_len = ext.ee_len;
            ee->ee_start_lo = ext.ee_start_lo;
            ee->ee_start_hi = ext.ee_start_hi;

            eh->eh_entries++;
            return true;
        }
    } else {
        if (AddExtentDepth(eh, 0, ext)) {
            _ds->Println("true");
            return true;
        } else {
            _ds->Println("false");
            return false;
        }
    }
    return false;
}