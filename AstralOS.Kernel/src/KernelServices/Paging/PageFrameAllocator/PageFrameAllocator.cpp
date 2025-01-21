#include "PageFrameAllocator.h"

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

	total_pages = GetMemorySize(mMap, mMapSize, mMapDescSize) / 4096;

    void* largestFreeMemSeg = NULL;
    size_t largestFreeMemSegSize = 0;

    for (size_t i = 0; i < mMapSize / mMapDescSize; i++) {
        EFI_MEMORY_DESCRIPTOR* descriptor = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)map + (i * mMapDescSize));

        if (descriptor->Type == 7) { // descriptor->type = EfiConventionalMemory
            if (descriptor->NumberOfPages * 4096 > largestFreeMemSegSize) {
                largestFreeMemSeg = (void*)descriptor->PhysicalStart;
                largestFreeMemSegSize = descriptor->NumberOfPages * 4096;
            }
        }
    }

    uint64_t mMapEntries = mMapSize / mMapDescSize;

    uint64_t memorySize = GetMemorySize(mMap, mMapEntries, mMapDescSize);
    freeMemory = memorySize;
    uint64_t bitmapSize = memorySize / 4096 / 8 + 1;

    InitBitmap(bitmapSize, largestFreeMemSeg);

    for (int i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)mMap + (i * mMapDescSize));
        if (desc->Type != EfiConventionalMemory && desc->Type != EfiBootServicesCode && desc->Type != EfiBootServicesData) { // not efiConventionalMemory
            ReservePages((void*)desc->PhysicalStart, desc->NumberOfPages);
        }
    }
}

void PageFrameAllocator::InitBitmap(size_t bitmapSize, void* bufferAddress) {
    page_bitmap.size = bitmapSize;
    page_bitmap.buffer = (uint8_t*)bufferAddress;
	for (size_t i = 0; i < (page_bitmap.size + 7) / 8; i++) {
		page_bitmap.buffer[i] = 0;
	}
}

void PageFrameAllocator::LockPage(void* address) {
    uint64_t index = (uint64_t)address / 4096;
    if (page_bitmap[index] == true) return;
    page_bitmap.Set(index, true);
    freeMemory -= 4096;
    usedMemory += 4096;
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
    uint64_t index = (uint64_t)address / 4096;
    if (page_bitmap[index] == false) return;
    page_bitmap.Set(index, false);
    freeMemory += 4096;
    usedMemory -= 4096;
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
    uint64_t index = (uint64_t)address / 4096;
    if (page_bitmap[index] == true) return;
    page_bitmap.Set(index, true);
    freeMemory -= 4096;
    reservedMemory += 4096;
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
    uint64_t index = (uint64_t)address / 4096;
    if (page_bitmap[index] == false) return;
    page_bitmap.Set(index, false);
    freeMemory += 4096;
    reservedMemory -= 4096;
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
        if (page_bitmap[index] == true) continue;
        LockPage((void*)(index * 4096));
        return (void*)(index * 4096);
    }

    return NULL;
}