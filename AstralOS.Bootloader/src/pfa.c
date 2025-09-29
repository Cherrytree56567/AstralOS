#include "pfa.h"

int Bitmap_Get(Bitmap* bmp, uint64_t index) {
    if (index >= bmp->size * 8) {
        Print((CHAR16*)L"Bitmap OOB read at index: 0x%lx", index);
        return 0;
    }
    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;
    return (bmp->buffer[byteIndex] & bitIndexer) > 0;
}

int Bitmap_Set(Bitmap* bmp, uint64_t index, int value) {
    if (index >= bmp->size * 8) {
        Print((CHAR16*)L"Bitmap OOB write at index: 0x%lx", index);
        return 0;
    }
    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;
    bmp->buffer[byteIndex] &= ~bitIndexer;
    if (value) {
        bmp->buffer[byteIndex] |= bitIndexer;
    }
    return 1;
}

void PFA_ReadEFIMemoryMap(PageFrameAllocator* pfa, EFI_MEMORY_DESCRIPTOR* map, size_t MapSize, size_t MapDescSize) {
    Print(L"Hi\n");
    if (pfa->initialized) return;
    Print(L"Reading memory map...\n");
    global_pfa = pfa;
    pfa->usedMemory = 0;
    pfa->freeMemory = 0;
    pfa->reservedMemory = 0;
    pfa->initialized = 1;

    pfa->mMap = map;
    pfa->mMapSize = MapSize;
    pfa->mMapDescSize = MapDescSize;

    uint64_t maxPhysAddr = 0;
    uint64_t mMapEntries = pfa->mMapSize / pfa->mMapDescSize;

    pfa->bitmapBase = 0xFFFFFFFFFFFFFFFFULL;
    for (uint64_t i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)pfa->mMap + i * pfa->mMapDescSize);
        if (desc->PhysicalStart < pfa->bitmapBase)
            pfa->bitmapBase = desc->PhysicalStart;
        uint64_t end = desc->PhysicalStart + desc->NumberOfPages * 4096;
        if (end > maxPhysAddr)
            maxPhysAddr = end;
    }
    Print((CHAR16*)L"Max Phys Addr: 0x%lx\n", pfa->bitmapBase);
    pfa->total_pages = (maxPhysAddr - pfa->bitmapBase + 0xFFF) / 4096;
    if (pfa->bitmapBase == 0xFFFFFFFFFFFFFFFFULL) {
        Print((CHAR16*)L"Invalid memory map\n");
        pfa->initialized = 0;
        return;
    }

    void* largestFreeMemSeg = NULL;
    size_t largestFreeMemSegSize = 0;

    for (uint64_t i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)pfa->mMap + (i * pfa->mMapDescSize));
        if (desc->Type == 7) { // EfiConventionalMemory
            if (desc->NumberOfPages * 4096 > largestFreeMemSegSize) {
                largestFreeMemSeg = (void*)desc->PhysicalStart;
                largestFreeMemSegSize = desc->NumberOfPages * 4096;
            }
        }
    }

    if (largestFreeMemSeg == NULL) {
        Print((CHAR16*)L"No suitable memory segment found for bitmap initialization.");
        pfa->initialized = 0;
        return;
    }

    uint64_t bitmapSize = (pfa->total_pages + 7) / 8;

    PFA_InitBitmap(pfa, bitmapSize, largestFreeMemSeg);
    PFA_LockPages(pfa, largestFreeMemSeg, (bitmapSize + 4095) / 4096);

    for (uint64_t i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)pfa->mMap + (i * pfa->mMapDescSize));
        if (desc->Type != EfiConventionalMemory) {
            PFA_ReservePages(pfa, (void*)desc->PhysicalStart, desc->NumberOfPages);
        }
    }
}

void PFA_InitBitmap(PageFrameAllocator* pfa, size_t bitmapSize, void* bufferAddress) {
    pfa->page_bitmap.size = bitmapSize;
    pfa->page_bitmap.buffer = (uint8_t*)bufferAddress;
    for (size_t i = 0; i < pfa->page_bitmap.size; i++) {
        pfa->page_bitmap.buffer[i] = 0;
    }
}

void PFA_LockPage(PageFrameAllocator* pfa, void* address) {
    uint64_t index = (((uint64_t)address) - pfa->bitmapBase) / 4096;
    if (Bitmap_Get(&pfa->page_bitmap, index)) return;
    if (Bitmap_Set(&pfa->page_bitmap, index, 1)) {
        pfa->freeMemory -= 4096;
        pfa->usedMemory += 4096;
    } else {
        Print((CHAR16*)L"Failed to lock page at address: 0x%lx", (uint64_t)address);
    }
}

void PFA_LockPages(PageFrameAllocator* pfa, void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        PFA_LockPage(pfa, currentPageAddress);
    }
}

void PFA_FreePage(PageFrameAllocator* pfa, void* address) {
    uint64_t index = (((uint64_t)address) - pfa->bitmapBase) / 4096;
    if (!Bitmap_Get(&pfa->page_bitmap, index)) return;
    if (Bitmap_Set(&pfa->page_bitmap, index, 0)) {
        pfa->freeMemory += 4096;
        pfa->usedMemory -= 4096;
    } else {
        Print((CHAR16*)L"Failed to free page at address: 0x%lx", (uint64_t)address);
    }
}

void PFA_FreePages(PageFrameAllocator* pfa, void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        PFA_FreePage(pfa, currentPageAddress);
    }
}

void PFA_ReservePage(PageFrameAllocator* pfa, void* address) {
    uint64_t index = (((uint64_t)address) - pfa->bitmapBase) / 4096;
    if (Bitmap_Get(&pfa->page_bitmap, index)) return;
    if (Bitmap_Set(&pfa->page_bitmap, index, 1)) {
        pfa->freeMemory -= 4096;
        pfa->reservedMemory += 4096;
    } else {
        Print((CHAR16*)L"Failed to reserve page at address: 0x%lx", (uint64_t)address);
    }
}

void PFA_ReservePages(PageFrameAllocator* pfa, void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        PFA_ReservePage(pfa, currentPageAddress);
    }
}

void PFA_UnReservePage(PageFrameAllocator* pfa, void* address) {
    uint64_t index = (((uint64_t)address) - pfa->bitmapBase) / 4096;
    if (!Bitmap_Get(&pfa->page_bitmap, index)) return;
    if (Bitmap_Set(&pfa->page_bitmap, index, 0)) {
        pfa->freeMemory += 4096;
        pfa->reservedMemory -= 4096;
    } else {
        Print((CHAR16*)L"Failed to unreserve page at address: 0x%lx", (uint64_t)address);
    }
}

void PFA_UnReservePages(PageFrameAllocator* pfa, void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        PFA_UnReservePage(pfa, currentPageAddress);
    }
}

uint64_t PFA_GetFreeRAM(PageFrameAllocator* pfa) {
    return pfa->freeMemory;
}

uint64_t PFA_GetUsedRAM(PageFrameAllocator* pfa) {
    return pfa->usedMemory;
}

uint64_t PFA_GetReservedRAM(PageFrameAllocator* pfa) {
    return pfa->reservedMemory;
}

void* PFA_RequestPage(PageFrameAllocator* pfa) {
    for (uint64_t index = 0; index < pfa->total_pages; index++) {
        if (Bitmap_Get(&pfa->page_bitmap, index)) continue;
        void* addr = (void*)(pfa->bitmapBase + index * 4096);
        PFA_LockPage(pfa, addr);
        return addr;
    }
    return NULL;
}

void* RequestPage() {
    if (isPFA) {
        return PFA_RequestPage(&global_pfa);
    } else {
        EFI_STATUS status;
        EFI_PHYSICAL_ADDRESS page; // 64-bit integer

        status = gBS->AllocatePages(
            AllocateAnyPages,       // allocation type
            EfiLoaderData,  // memory type
            5,                      // number of pages
            &page                   // pointer to EFI_PHYSICAL_ADDRESS
        );

        if (EFI_ERROR(status)) {
            if (status == EFI_OUT_OF_RESOURCES)
        {
            Print(L"EFI_OUT_OF_RESOURCES\n");
        }
        else if (status == EFI_INVALID_PARAMETER)
        {
            Print(L"EFI_INVALID_PARAMETER\n");
        }
        else if (status == EFI_NOT_FOUND)
        {
            Print(L"EFI_NOT_FOUND\n");
        }
            Print(L"Failed to allocate page 0x%lx\n", status);
            return NULL;
        }

        Print(L"Got page at physical address: 0x%lx\n", (EFI_PHYSICAL_ADDRESS)page);
        return page;
    }
    return NULL;
}