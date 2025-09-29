#pragma once
#include <efi.h>
#include <efilib.h>
#include <efistdarg.h>
#include <libsmbios.h>
#include <stddef.h>

#define SafeFree(p) do { FreePool(p); p = NULL;} while(0)

static EFI_HANDLE gImageHandle = NULL;
static EFI_SYSTEM_TABLE* ggST = NULL;

#define true 1
#define false 0

/*
* Framebuffer Struct
*
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-3-Graphics-Output-Protocol/gnu-efi/bootloader/main.c
*/
typedef struct {
    void* BaseAddress;
    size_t BufferSize;
    unsigned int Width;
    unsigned int Height;
    unsigned int PixelsPerScanLine;
} Framebuffer;

typedef struct {
    Framebuffer pFramebuffer;
    EFI_MEMORY_DESCRIPTOR* mMap;
    uint64_t mMapSize;
    uint64_t mMapDescSize;
    void* rsdp;
    void* initrdBase;
    uint64_t initrdSize;
} BootInfo;

enum FSType {
    NTFS = 0,
    EXFAT = 1,
    EXT = 2
};
