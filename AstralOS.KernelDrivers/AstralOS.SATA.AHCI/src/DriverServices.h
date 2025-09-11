#pragma once
#include <cstdint>
#include <cstddef>

class BaseDriverFactory;
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
    char* (*strdup)(const char* str);

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
    uint16_t (*ConfigReadWord)(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    void (*ConfigWriteWord)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
    void (*ConfigWriteDWord)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
    uint8_t (*ConfigReadByte)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t (*ConfigReadDWord)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    /*
     * Driver Stuff
    */
    void (*RegisterDriver)(BaseDriverFactory* factory);
};

struct DriverInfo {
    char* name; // Driver Name (for debugging and stuff)
    int verMaj; // Version Major
    int verMin; // Version Minor
    int exCode; // Exit Code

    //            Name     Ver Maj    Ver Min   Exit Code
    DriverInfo(char* nam, int verMa, int verMi, int Ecode) 
             : name(name), verMaj(verMa), verMin(verMi), exCode(Ecode) {}
};