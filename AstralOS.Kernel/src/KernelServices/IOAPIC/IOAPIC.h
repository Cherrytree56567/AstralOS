#pragma once
#include <cstdint>
#include <stdint.h>
#include "../BasicConsole/BasicConsole.h"
#include "../../Utils/cstr/cstr.h"

/*
 * This code is from the OSDev Wiki
 * https://wiki.osdev.org/IOAPIC
 */
#define IOAPICID          0x00
#define IOAPICVER         0x01
#define IOAPICARB         0x02
#define IOAPICREDTBL(n)   (0x10 + 2 * n) // lower-32bits (add +1 for upper 32-bits)

enum DeliveryMode {
    EDGE = 0,
    LEVEL = 1
};

enum DestinationMode {
    PHYSICAL = 0,
    LOGICAL = 1
};

union RedirectionEntry {
    struct {
        uint64_t vector       : 8;
        uint64_t delvMode     : 3;
        uint64_t destMode     : 1;
        uint64_t delvStatus   : 1;
        uint64_t pinPolarity  : 1;
        uint64_t remoteIRR    : 1;
        uint64_t triggerMode  : 1;
        uint64_t mask         : 1;
        uint64_t reserved     : 39;
        uint64_t destination  : 8;
    } __attribute__((packed));
    struct {
        uint32_t lowerDword;
        uint32_t upperDword;
    } __attribute__((packed));
};

class IOAPIC {
public:
    IOAPIC() {}

    void Initialize(BasicConsole* bc, unsigned long virtAddr, unsigned long apicId, unsigned long gsib);

    RedirectionEntry getRedirEntry(unsigned char entNo);
    void writeRedirEntry(unsigned char entNo, RedirectionEntry *entry);
    
    unsigned char id(){ return (apicId); }
    unsigned char ver(){ return (apicVer); }
    unsigned char redirectionEntries(){ return (redirEntryCnt); }
    unsigned long globalInterruptBase(){ return (globalIntrBase); }
private:
    uint32_t read(unsigned char regOff);
    void write(unsigned char regOff, uint32_t data);

    BasicConsole* basicConsole;
    unsigned long physRegs;
    unsigned long virtAddr;
    unsigned char apicId;
    unsigned char apicVer;
    unsigned char redirEntryCnt;
    unsigned long globalIntrBase;
};