#pragma once
#include <stdint.h>

/*
* EFI_MEMORY_DESCRIPTOR
*
* The EFI_MEMORY_DESCRIPTOR structure was found in the UEFI Specification.
* It is used to describe a region of memory in the EFI memory map.
* The UEFI memory map is an array of EFI_MEMORY_DESCRIPTOR structures.
* The memory map is passed to the OS by the firmware when the OS is loaded.
* The memory map is used by the OS to determine which regions of memory are available for use.
* The memory map is also used by the OS to determine which regions of memory are reserved for use by the OS.
*/
typedef struct {
    uint32_t Type;
    uint32_t Pad;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiUnacceptedMemoryType,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

extern const char* EFI_MEMORY_TYPE_STRINGS[];

uint64_t GetMemorySize(EFI_MEMORY_DESCRIPTOR* mMap, uint64_t mMapEntries, uint64_t mMapDescSize);

extern "C" void memsetC(void* start, uint8_t value, uint64_t num);
void memset(void* start, uint8_t value, uint64_t num);