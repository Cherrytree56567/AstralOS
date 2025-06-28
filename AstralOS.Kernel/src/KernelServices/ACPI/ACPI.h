#pragma once
#include <cstdint>
#include "../BasicConsole/BasicConsole.h"
#include "../../cstr/cstr.h"

/*
 * The implementation of the RSDP is found on the OSDev Wiki:
 * https://wiki.osdev.org/RSDP
*/

/*
 * ACPI Version 1.0 RSDP struct.
*/
struct RSDP {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__ ((packed));

/*
 * ACPI Version 2.0 RSDP struct.
*/
struct XSDP {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;      // deprecated since version 2.0

    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

/*
 * The implementation of the XSDT is found on the OSDev Wiki:
 * https://wiki.osdev.org/XSDT
*/
struct ACPISDTHeader {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
};

struct XSDT {
    struct ACPISDTHeader h;
    uint64_t Entries[];
};

/*
 * We aren't going to implement the RSDT struct bc
 * it isn't necessary for a 64-Bit Kernel.
*/
class ACPI {
public:
   ACPI() {}

   void Initialize(BasicConsole* bc, void* rsdpAddr);
   bool doChecksum(ACPISDTHeader *tableHeader);

   void* GetFACP();
   void* GetMADT();
   void* GetDSDT();
   void* GetRSDT();
   void* GetSRAT();
   void* GetSSDT();
private:
   RSDP* rsdp;
   XSDP* xsdp;
   BasicConsole* basicConsole;
   bool isRSDP;
   XSDT* xsdt;
};