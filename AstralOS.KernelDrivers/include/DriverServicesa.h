#pragma once
#include <cstdint>
#include <cstddef>

struct DriverServices {
    /*
     * Debugging
    */
    void (*Print)(const char* str);
    void (*Println)(const char* str);

    /*
     * Memory
    */
    void* (*RequestPage)();
    void (*LockPage)(void* address);
    void (*LockPages)(void* address, uint64_t pageCount);
    void (*FreePage)(void* address);
    void (*FreePages)(void* address, uint64_t pageCount);
    void (*MapMemory)(void* virtualMemory, void* physicalMemory);
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    void (*strdup)(const char* str);

    /*
     * IRQs
    */
    void (*SetDescriptor)(uint8_t vector, void* isr, uint8_t flags);

    /*
     * PCI/e
    */
    bool (*PCIExists)();
    bool (*PCIeExists)();
    bool (*EnableMSI)(uint8_t bus, uint8_t device, uint8_t function, uint8_t vector);
    bool (*EnableMSIx)(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector);
};

struct DriverInfo {
    const char* name; // Driver Name (for debugging and stuff)
    const int verMaj; // Version Major
    const int verMin; // Version Minor
    const int exCode; // Exit Code

    //            Name     Ver Min    Ver Maj   Exit Code
    DriverInfo(char* nam, int verMi, int verMa, int Ecode) 
             : name(name), verMaj(verMa), verMin(verMi), exCode(Ecode) {}
};