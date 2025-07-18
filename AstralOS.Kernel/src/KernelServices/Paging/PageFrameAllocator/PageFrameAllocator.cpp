#include "PageFrameAllocator.h"

/*
 * This code is mostly from Poncho OS.
*/
PageFrameAllocator::PageFrameAllocator() {
	usedMemory = 0;
	freeMemory = 0;
	reservedMemory = 0;
}

void PageFrameAllocator::ReadEFIMemoryMap(EFI_MEMORY_DESCRIPTOR* map, size_t MapSize, size_t MapDescSize) {
    if (initialized) return;

    initialized = true;

	mMap = map;
	mMapSize = MapSize;
	mMapDescSize = MapDescSize;

    uint64_t maxPhysAddr = 0;
    uint64_t mMapEntries = mMapSize / mMapDescSize;

    bitmapBase = 0xFFFFFFFFFFFFFFFFULL;
    for (uint64_t i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + i * mMapDescSize);
        if (desc->PhysicalStart < bitmapBase)
            bitmapBase = desc->PhysicalStart;
        uint64_t end = desc->PhysicalStart + desc->NumberOfPages * 4096;
        if (end > maxPhysAddr)
            maxPhysAddr = end;
    }
    total_pages = (maxPhysAddr - bitmapBase + 0xFFF) / 4096;

    void* largestFreeMemSeg = NULL;
    size_t largestFreeMemSegSize = 0;

    for (int i = 0; i < mMapEntries; i++){
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));
        if (desc->Type == 7){ // type = EfiConventionalMemory
            if (desc->NumberOfPages * 4096 > largestFreeMemSegSize) {
                largestFreeMemSeg = (void*)desc->PhysicalStart;
                largestFreeMemSegSize = desc->NumberOfPages * 4096;
            }
        }
    }

    if (largestFreeMemSeg == NULL) {
        basicConsole->Println("No suitable memory segment found for bitmap initialization.");
        initialized = false;
        return;
    }

    uint64_t bitmapSize = total_pages / 8 + 1;

    InitBitmap(bitmapSize, largestFreeMemSeg);

    LockPages(page_bitmap.buffer, (bitmapSize + 4095) / 4096);

    for (int i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));
        if (desc->Type != EfiConventionalMemory) { // not efiConventionalMemory
            ReservePages((void*)desc->PhysicalStart, desc->NumberOfPages);
        }
    }
}

void PageFrameAllocator::InitBitmap(size_t bitmapSize, void* bufferAddress) {
    page_bitmap.size = bitmapSize;
    page_bitmap.buffer = (uint8_t*)bufferAddress;
	for (size_t i = 0; i < page_bitmap.size; i++) {
        page_bitmap.buffer[i] = 0;
    }
}

void PageFrameAllocator::LockPage(void* address) {
    uint64_t index = ((uint64_t)address) / 4096;
    if (page_bitmap[index] == true) return;
    if (page_bitmap.Set(index, true)) {
        freeMemory -= 4096;
        usedMemory += 4096;
    }  else {
        basicConsole->Print("Failed to lock page at address: ");
        basicConsole->Println(to_hstring((uint64_t)address));
    }
}

void PageFrameAllocator::LockPages(void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        // Calculate the address of the current page.
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        // Call LockPage for the current page.
        LockPage(currentPageAddress);
    }
}

void PageFrameAllocator::FreePage(void* address) {
    uint64_t index = ((uint64_t)address) / 4096;
    if (page_bitmap[index] == false) return;
    if (page_bitmap.Set(index, false)) {
        freeMemory += 4096;
        usedMemory -= 4096;
    } else {
        basicConsole->Print("Failed to free page at address: ");
        basicConsole->Println(to_hstring((uint64_t)address));
    }
}

void PageFrameAllocator::FreePages(void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        // Calculate the address of the current page.
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        // Call LockPage for the current page.
        FreePage(currentPageAddress);
    }
}

void PageFrameAllocator::ReservePage(void* address) {
    uint64_t index = ((uint64_t)address) / 4096;
    if (page_bitmap[index] == true) return;
    if (page_bitmap.Set(index, true)) {
        freeMemory -= 4096;
        reservedMemory += 4096;
    } else {
        basicConsole->Print("Failed to reserve page at address: ");
        basicConsole->Println(to_hstring((uint64_t)address));
    }
}

void PageFrameAllocator::ReservePages(void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        // Calculate the address of the current page.
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        // Call LockPage for the current page.
        ReservePage(currentPageAddress);
    }
}

void PageFrameAllocator::UnReservePage(void* address) {
    uint64_t index = ((uint64_t)address) / 4096;
    if (page_bitmap[index] == false) return;
    if (page_bitmap.Set(index, false)) {
        freeMemory += 4096;
        reservedMemory -= 4096;
    } else {
        basicConsole->Print("Failed to unreserve page at address: ");
        basicConsole->Println(to_hstring((uint64_t)address));
    }
}

void PageFrameAllocator::UnReservePages(void* address, uint64_t pageCount) {
    for (uint64_t i = 0; i < pageCount; i++) {
        // Calculate the address of the current page.
        void* currentPageAddress = (void*)((uint64_t)address + (i * 4096));
        // Call LockPage for the current page.
        UnReservePage(currentPageAddress);
    }
}

uint64_t PageFrameAllocator::GetFreeRAM() {
    return freeMemory;
}

uint64_t PageFrameAllocator::GetUsedRAM() {
    return usedMemory;
}

uint64_t PageFrameAllocator::GetReservedRAM() {
    return reservedMemory;
}

void* PageFrameAllocator::RequestPage() {
    for (uint64_t index = 0; index < page_bitmap.size * 8; index++) {
        if (index >= total_pages) {
            basicConsole->Print("OOB access at ");
            basicConsole->Println(to_hstring(index));
            return NULL;
        }
        if (page_bitmap[index] == true) continue;
        LockPage((void*)(index * 4096));
        return (void*)(index * 4096);
    }

    return NULL;
}