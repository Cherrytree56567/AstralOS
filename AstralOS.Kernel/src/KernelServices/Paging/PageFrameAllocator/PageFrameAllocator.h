#pragma once
#include "EFIMemoryMap/EFIMemoryMap.h"
#include "Bitmap/Bitmap.h"
#include "../../BasicConsole/BasicConsole.h"
#include "../../../cstr/cstr.h"

#define PAGE_SIZE 4096

class PageFrameAllocator {
public:
    PageFrameAllocator();

    void Initialise(BasicConsole* console) {
        this->basicConsole = console;
    }

    void ReadEFIMemoryMap(EFI_MEMORY_DESCRIPTOR* mMap, size_t mMapSize, size_t mMapDescSize);
    void LockPage(void* address);
    void LockPages(void* address, uint64_t pageCount);
    void FreePage(void* address);
    void FreePages(void* address, uint64_t pageCount);
    void* RequestPage();

    uint64_t GetFreeRAM();
    uint64_t GetUsedRAM();
    uint64_t GetReservedRAM();

    Bitmap GetBitmap() {
        return page_bitmap;
    }
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
    uint64_t bitmapBase;
    uint64_t total_pages;
    bool initialized = false;
    uint64_t freeMemory;
    uint64_t reservedMemory;
    uint64_t usedMemory;
    BasicConsole* basicConsole;
};