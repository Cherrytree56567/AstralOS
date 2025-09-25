#pragma once
#include <cstdint>
#include "../BasicConsole/BasicConsole.h"
#include "../../Utils/cstr/cstr.h"
#include "../../Utils/cpu.h"

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

struct RSDT {
    struct ACPISDTHeader h;
    uint32_t Entries[];
};

struct XSDT {
    struct ACPISDTHeader h;
    uint64_t Entries[];
};

struct MADT {
    ACPISDTHeader header;
    uint32_t LocalAPICAddress;
    uint32_t Flags;
    uint8_t Entries[];
};

struct MADTEntryHeader {
    uint8_t Type;
    uint8_t Length;
};

struct MADT_ProcessorLocalAPIC {
    MADTEntryHeader Header;
    uint8_t ACPI_ProcessorID;
    uint8_t APIC_ID;
    uint32_t Flags;
} __attribute__((packed));

struct MADT_IOAPIC {
    MADTEntryHeader Header;
    uint8_t IOAPIC_ID;
    uint8_t Reserved;
    uint32_t IOAPIC_Addr;
    uint32_t GSI_Base;
} __attribute__((packed));

struct BGRT {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
    uint16_t VersionID;
    uint8_t Status;
    uint8_t ImageType;
    uint64_t ImageAddress;
    uint32_t ImageXOffset;
    uint32_t ImageYOffset;
};

struct MCFGEntry {
    uint64_t BaseAddr;
    uint16_t PCISegmentGroupNum;
    uint8_t StartPCIBusNum;
    uint8_t EndPCIBusNum;
    uint32_t Reserved;
};

struct MCFG {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    uint8_t OEMID[6];
    uint8_t OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
    uint64_t Reserved;
    MCFGEntry entries[];
}__attribute__((packed));

/*
 * Code from the OSDev Wiki:
 * https://wiki.osdev.org/FADT
*/
struct GenericAddressStructure {
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
} __attribute__((packed));

struct FADT {
    ACPISDTHeader h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t Reserved;

    uint8_t PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t AcpiEnable;
    uint8_t AcpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t PM1EventLength;
    uint8_t PM1ControlLength;
    uint8_t PM2ControlLength;
    uint8_t PMTimerLength;
    uint8_t GPE0Length;
    uint8_t GPE1Length;
    uint8_t GPE1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t BootArchitectureFlags;

    uint8_t Reserved2;
    uint32_t Flags;

    // 12 byte structure; see below for details
    GenericAddressStructure ResetReg;

    uint8_t ResetValue;
    uint8_t Reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t X_FirmwareControl;
    uint64_t X_Dsdt;

    GenericAddressStructure X_PM1aEventBlock;
    GenericAddressStructure X_PM1bEventBlock;
    GenericAddressStructure X_PM1aControlBlock;
    GenericAddressStructure X_PM1bControlBlock;
    GenericAddressStructure X_PM2ControlBlock;
    GenericAddressStructure X_PMTimerBlock;
    GenericAddressStructure X_GPE0Block;
    GenericAddressStructure X_GPE1Block;
};

/*
 * Found on the OSDev Wiki:
 * https://wiki.osdev.org/SRAT
*/
struct SRAT {
    char signature[4];   // Contains "SRAT"
    uint32_t length;     // Length of entire SRAT including entries
    uint8_t rev;         // 3
    uint8_t checksum;    // Entire table must sum to zero
    uint8_t OEMID[6];    // What do you think it is?
    uint64_t OEMTableID; // For the SRAT it's the manufacturer model ID
    uint32_t OEMRev;     // OEM revision for OEM Table ID
    uint32_t creatorID;  // Vendor ID of the utility used to create the table
    uint32_t creatorRev; // Blah blah
    
    uint8_t reserved[12];
} __attribute__((packed));

struct SRAT_proc_lapic {
    uint8_t type;      // 0x0 for this type of structure
    uint8_t length;    // 16
    uint8_t lo_DM;     // Bits [0:7] of the proximity domain
    uint8_t APIC_ID;   // Processor's APIC ID
    uint32_t flags;    // Haha the most useless thing ever
    uint8_t SAPIC_EID; // The processor's local SAPIC EID. Don't even bother.
    uint8_t hi_DM[3];  // Bits [8:31] of the proximity domain
    uint32_t _CDM;     // The clock domain which the processor belongs to (more jargon)
} __attribute__((packed));

struct SRAT_mem_struct {
    uint8_t type;         // 0x1 for this type of structure
    uint8_t length;       // 40
    uint32_t domain;      // The domain to which this memory region belongs to
    uint8_t reserved1[2]; // Reserved
    uint32_t lo_base;     // Low 32 bits of the base address of the memory range
    uint32_t hi_base;     // High 32 bits of the base address of the memory range
    uint32_t lo_length;   // Low 32 bits of the length of the range
    uint32_t hi_length;   // High 32 bits of the length
    uint8_t reserved2[4]; // Reserved
    uint32_t flags;       // Flags
    uint8_t reserved3[8]; // Reserved
} __attribute__ ((packed));

struct SRAT_proc_lapic2 {
    uint8_t type;         // 0x2 for this type of structure
    uint8_t length;       // 24
    uint8_t reserved1[2]; // Must be zero
    uint32_t domain;      // The proximity domain which the logical processor belongs to
    uint32_t x2APIC_ID;   // Processor's x2APIC ID
    uint32_t flags;       // Haha the most useless thing ever
    uint32_t _CDM;        // The clock domain which the processor belongs to (more jargon)
    uint8_t reserved2[4]; // Reserved.
} __attribute__((packed));

/*
 * We aren't going to implement the RSDT struct bc
 * it isn't necessary for a 64-Bit Kernel.
*/
class ACPI {
public:
    ACPI() {}

    void Initialize(BasicConsole* bc, void* rsdpAddr);
    bool doChecksum(ACPISDTHeader *tableHeader);

    FADT* GetFADT();
    MADT* GetMADT();
    BGRT* GetBGRT();
    RSDT* GetRSDT();
    SRAT* GetSRAT();
    MCFG* GetMCFG();

    MADT_ProcessorLocalAPIC* GetProcessorLocalAPIC();
    MADT_IOAPIC* GetIOAPIC();
private:
    RSDP* rsdp;
    XSDP* xsdp;
    BasicConsole* basicConsole;
    bool isRSDP;
    XSDT* xsdt;
};