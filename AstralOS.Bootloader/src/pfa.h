#pragma once
#include "main.h"
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

/*
 * Refactored by GPT
*/
extern const char* EFI_MEMORY_TYPE_STRINGS[];

typedef struct {
    uint8_t* buffer;
    size_t size;
} Bitmap;

int Bitmap_Get(Bitmap* bmp, uint64_t index);
int Bitmap_Set(Bitmap* bmp, uint64_t index, int value);

typedef struct {
    EFI_MEMORY_DESCRIPTOR* mMap;
    size_t mMapSize;
    size_t mMapDescSize;
    Bitmap page_bitmap;
    uint64_t bitmapBase;
    uint64_t total_pages;
    int initialized;
    uint64_t freeMemory;
    uint64_t reservedMemory;
    uint64_t usedMemory;
} PageFrameAllocator;

static PageFrameAllocator* global_pfa;
static BOOLEAN isPFA = false;

/* Public API */
void PFA_ReadEFIMemoryMap(PageFrameAllocator* pfa, EFI_MEMORY_DESCRIPTOR* mMap, size_t mMapSize, size_t mMapDescSize);
void PFA_LockPage(PageFrameAllocator* pfa, void* address);
void PFA_LockPages(PageFrameAllocator* pfa, void* address, uint64_t pageCount);
void PFA_FreePage(PageFrameAllocator* pfa, void* address);
void PFA_FreePages(PageFrameAllocator* pfa, void* address, uint64_t pageCount);
void* PFA_RequestPage(PageFrameAllocator* pfa);

uint64_t PFA_GetFreeRAM(PageFrameAllocator* pfa);
uint64_t PFA_GetUsedRAM(PageFrameAllocator* pfa);
uint64_t PFA_GetReservedRAM(PageFrameAllocator* pfa);

Bitmap PFA_GetBitmap(PageFrameAllocator* pfa);

/* Private helpers */
void PFA_InitBitmap(PageFrameAllocator* pfa, size_t bitmapSize, void* bufferAddress);
void PFA_ReservePage(PageFrameAllocator* pfa, void* address);
void PFA_ReservePages(PageFrameAllocator* pfa, void* address, uint64_t pageCount);
void PFA_UnReservePage(PageFrameAllocator* pfa, void* address);
void PFA_UnReservePages(PageFrameAllocator* pfa, void* address, uint64_t pageCount);

void* RequestPage();