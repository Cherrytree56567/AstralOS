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