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