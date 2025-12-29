#include "GenericEXT4.h"

uint64_t GenericEXT4Device::CountExtents(ExtentHeader* hdr) {
    if (hdr->depth == 0) {
        return hdr->entries;
    }

    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    uint64_t total = 0;

    Extent* ext = (Extent*)(hdr + 1);

    uint64_t bufPhys = (uint64_t)_ds->RequestPage();
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

    for (uint16_t i = 0; i < hdr->entries; i++) {
        memset((void*)bufVirt, 0, 4096);
        uint64_t child_block = ((uint64_t)ext[i].startHigh << 32) | ext[i].startLow;

        for (uint64_t s = 0; s < sectorsPerBlock; s++) {
            uint64_t lba = child_block * sectorsPerBlock + s;
            if (!pdev->ReadSector(lba, (void*)((uint64_t)bufPhys + s * pdev->SectorSize()))) {
                _ds->Println("Failed to read sector");
                return 0;
            }
        }

        uint8_t* buf = (uint8_t*)bufVirt;
        ExtentHeader* child = (ExtentHeader*)buf;

        total += CountExtents(child);
    }

    _ds->UnMapMemory((void*)bufVirt);
    _ds->FreePage((void*)bufPhys);
    return total;
}

Extent** GenericEXT4Device::GetExtents(Inode* ind, ExtentHeader* exthdr, uint64_t& extentsCount) {
    uint64_t countExt = CountExtents(exthdr);
    Extent** extents = (Extent**)_ds->malloc(countExt * sizeof(Extent*));
    if (exthdr->magic != 0xF30A) {
        _ds->Println("NOT an extent inode");
    }

    if (exthdr->depth == 0) {
        _ds->Println("Depth is 0");
        Extent* ext = (Extent*)(exthdr + 1);

        for (uint16_t i = 0; i < exthdr->entries; i++) {
            Extent* copy = (Extent*)_ds->malloc(sizeof(Extent));
            *copy = ext[i];
            extents[extentsCount++] = copy;
        }

        return extents;
    }

    uint64_t bufPhys = (uint64_t)_ds->RequestPage();
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    Extent* ext = (Extent*)(exthdr + 1);

    for (int i = 0; i < exthdr->entries; i++) {
        memset((void*)bufVirt, 0, 4096);
        uint64_t block = ((uint64_t)ext[i].startHigh << 32) | ext[i].startLow;

        for (uint64_t s = 0; s < sectorsPerBlock; s++) {
            uint64_t lba = block * sectorsPerBlock + s;
            if (!pdev->ReadSector(lba, (void*)((uint64_t)bufPhys + s * pdev->SectorSize()))) {
                _ds->Println("Failed to read sector");
                return nullptr;
            }
        }

        uint8_t* buf = (uint8_t*)bufVirt;
        ExtentHeader* child = (ExtentHeader*)buf;

        extents = GetExtents(ind, child, extentsCount);
    }

    return extents;
}

/*
 * This func is GPT Generated.
 * I will rewrite this later.
*/
uint64_t GenericEXT4Device::ResolveExtentBlockFromHdr(ExtentHeader* hdr, uint64_t logicalBlock) {
    if (hdr->depth == 0) {
        Extent* ext = (Extent*)(hdr + 1);

        for (uint16_t i = 0; i < hdr->entries; i++) {
            uint32_t logicalStart = ext[i].block;
            uint32_t len = ext[i].len & 0x7FFF;

            if (logicalBlock >= logicalStart &&
                logicalBlock < logicalStart + len) {

                uint64_t physStart =
                    ((uint64_t)ext[i].startHigh << 32) | ext[i].startLow;

                return physStart + (logicalBlock - logicalStart);
            }
        }

        return 0;
    }

    ExtentIDX* idx = (ExtentIDX*)(hdr + 1);

    for (int i = hdr->entries - 1; i >= 0; i--) {
        if (logicalBlock >= idx[i].Block) {
            uint64_t nextPhys = ((uint64_t)idx[i].LeafHigh << 32) | idx[i].LeafLow;

            uint64_t blockSize = 1024ULL << superblock->BlockSize;
            uint64_t sectors = blockSize / pdev->SectorSize();

            void* tmp = _ds->RequestPage();
            uint64_t tmpPhys = (uint64_t)tmp;
            uint64_t tmpVirt = tmpPhys + 0xFFFFFFFF00000000;
            _ds->MapMemory((void*)tmpVirt, (void*)tmpPhys, false);

            for (uint64_t s = 0; s < sectors; s++) {
                if (!pdev->ReadSector(nextPhys * sectors + s, (void*)(tmpPhys + s * pdev->SectorSize()))) {
                    return 0;
                }
            }

            return ResolveExtentBlockFromHdr((ExtentHeader*)tmpVirt, logicalBlock);
        }
    }

    return 0;
}

/*
 * Also GPT Generated and will be rewritten later
*/
uint64_t GenericEXT4Device::ResolveExtentBlock(Inode* inode, uint64_t logicalBlock) {
    ExtentHeader* hdr = (ExtentHeader*)inode->DBP;

    if (hdr->magic != 0xF30A) return 0;

    if (hdr->depth == 0) {
        Extent* ext = (Extent*)(hdr + 1);

        for (uint16_t i = 0; i < hdr->entries; i++) {
            uint32_t logicalStart = ext[i].block;
            uint32_t len = ext[i].len & 0x7FFF;

            if (logicalBlock >= logicalStart &&
                logicalBlock < logicalStart + len) {

                uint64_t physStart = ((uint64_t)ext[i].startHigh << 32) | ext[i].startLow;

                return physStart + (logicalBlock - logicalStart);
            }
        }

        return 0;
    }

    ExtentIDX* idx = (ExtentIDX*)(hdr + 1);

    for (int i = hdr->entries - 1; i >= 0; i--) {
        if (logicalBlock >= idx[i].Block) {
            uint64_t nextPhys = ((uint64_t)idx[i].LeafHigh << 32) | idx[i].LeafLow;

            uint64_t blockSize = 1024ULL << superblock->BlockSize;
            uint64_t sectors = blockSize / pdev->SectorSize();

            void* tmp = _ds->RequestPage();
            uint64_t tmpPhys = (uint64_t)tmp;
            uint64_t tmpVirt = tmpPhys + 0xFFFFFFFF00000000;
            _ds->MapMemory((void*)tmpVirt, (void*)tmpPhys, false);

            for (uint64_t s = 0; s < sectors; s++) {
                if (!pdev->ReadSector(nextPhys * sectors + s, (void*)(tmpPhys + s * pdev->SectorSize()))) {
                    return 0;
                }
            }

            ExtentHeader* nextHdr = (ExtentHeader*)tmpVirt;
            return ResolveExtentBlockFromHdr(nextHdr, logicalBlock);
        }
    }

    return 0;
}