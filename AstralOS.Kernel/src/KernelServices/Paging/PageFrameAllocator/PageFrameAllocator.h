#pragma once
#include "EFIMemoryMap/EFIMemoryMap.h"
#include "Bitmap/Bitmap.h"

class PageFrameAllocator {
public:
    PageFrameAllocator();

    void ReadEFIMemoryMap(EFI_MEMORY_DESCRIPTOR* mMap, size_t mMapSize, size_t mMapDescSize);
    void LockPage(void* address);
    void LockPages(void* address, uint64_t pageCount);
    void FreePage(void* address);
    void FreePages(void* address, uint64_t pageCount);
    void* RequestPage();

    uint64_t GetFreeRAM();
    uint64_t GetUsedRAM();
    uint64_t GetReservedRAM();
private:
    void InitBitmap(size_t bitmapSize, void* bufferAddress);
    void ReservePage(void* address);
    void ReservePages(void* address, uint64_t pageCount);
    void UnReservePage(void* address);
    void UnReservePages(void* address, uint64_t pageCount);

    EFI_MEMORY_DESCRIPTOR* mMap;
    size_t mMapSize;
    size_t mMapDescSize;
    Bitmap page_bitmap;
    uint64_t total_pages;
    bool initialized = false;
    uint64_t freeMemory;
    uint64_t reservedMemory;
    uint64_t usedMemory;
};