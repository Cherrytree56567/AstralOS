#pragma once
#include <cstdint>
#include "../CPUutils/cpuid.h"

/*
 * This code is from the OSDev Wiki:
 * https://wiki.osdev.org/PCI#The_PCI_Bus
*/
enum class HeaderType : uint8_t {
    General = 0x0,
    PCI_PCI_BRIDGE = 0x1,
    PCI_CARDBus_BRIDGE = 0x2
};

enum class DEVSEL : uint8_t {
    Fast = 0,
    Medium = 1,
    Slow = 2,
    Reserved = 3
};

struct MemSpaceBAR {
    uint32_t Identifier : 1;    // Always 0
    uint32_t Type : 2;
    uint32_t Prefetchable : 1;
    uint32_t BaseAddr : 28; // 16-Byte Aligned Base Address
} __attribute__((packed));

struct IOSpaceBAR {
    uint32_t Identifier : 1;    // Always 1
    uint32_t Reserved : 1;
    uint32_t BaseAddr : 30; // 4-Byte Aligned Base Address 
} __attribute__((packed));

struct GenericBAR {
    uint32_t Identifier : 1;
    uint32_t Reserved : 31;
} __attribute__((packed));

static_assert(sizeof(MemSpaceBAR) == 4, "BAR struct must be 32 bits");
static_assert(sizeof(IOSpaceBAR) == 4, "BAR struct must be 32 bits");
static_assert(sizeof(GenericBAR) == 4, "BAR struct must be 32 bits");

struct PCI_Header {
    uint16_t VendorID;      // Identifies the manufacturer
    uint16_t DeviceID;      // Identifies the Device
    uint16_t Command;       // Provides control over a device's ability to generate and respond to PCI cycles.
    uint16_t Status;        // A register used to record status information for PCI bus related events.
    uint8_t RevisionID;     // Specifies a revision identifier for a particular device.
    uint8_t ProgIF;         // Programming Interface Byte.
    uint8_t Subclass;       // A read-only register that specifies the specific function the device performs.
    uint8_t ClassCode;      // A read-only register that specifies the type of function the device performs.
    uint8_t CacheLineSize;  // Specifies the system cache line size in 32-bit units.
    uint8_t LatencyTimer;   // Specifies the latency timer in units of PCI bus clocks.
    uint8_t HeaderType;     // Identifies the layout of the rest of the header beginning at byte 0x10 of the header.
    uint8_t BIST;           // Represents that status and allows control of a devices BIST.

    /*
     * Command Register
    */
    void SetIOSpace(bool val);
    bool IOSpace();
    void SetMemorySpace(bool val);
    bool MemorySpace();
    void SetBusMaster(bool val);
    bool BusMaster();
    bool SpecialCycles();
    bool MemWrite();
    bool VGAPaletteSnoop();
    void SetParityErrorResponse(bool val);
    bool ParityErrorResponse();
    void SetSERREnable(bool val);
    bool SERREnable();
    bool FastBBEnable();
    void SetInterruptDisable(bool val);
    bool InterruptDisable();

    /*
     * Status Register
    */
    bool InterruptStatus();
    bool CapabilitiesList();
    bool MHzCapable();
    bool FastBBCapable();
    bool MasterDataParityError();
    DEVSEL DEVSELTiming();
    bool SignaledTargetAbort();
    bool RecievedTargetAbort();
    bool RecievedMasterAbort();
    bool SignaledSystemError();
    bool DetectedParityError();
    void SetMasterDataParityError();
    void SetSignaledTargetAbort();
    void SetRecievedTargetAbort();
    void SetRecievedMasterAbort();
    void SetSignaledSystemError();
    void SetDetectedParityError();

    HeaderType GetHeaderType(uint8_t raw);
    bool IsMultiFunction(uint8_t raw);

    /*
     * BIST
    */
    uint8_t GetCompletionCode();
    bool IsStartBIST();
    void SetStartBIST(bool start);
    bool IsBISTCapable();
} __attribute__((packed));

struct GeneralPCI_Header {
    PCI_Header pci;
    GenericBAR BAR0;                // Base address #0 (BAR0) 
    GenericBAR BAR1;                // Base address #1 (BAR1) 
    GenericBAR BAR2;                // Base address #2 (BAR2) 
    GenericBAR BAR3;                // Base address #3 (BAR3) 
    GenericBAR BAR4;                // Base address #4 (BAR4) 
    GenericBAR BAR5;                // Base address #5 (BAR5)
    uint32_t CardbusCISPointer;     // Points to the Card Information Structure and is used by devices that share silicon between CardBus and PCI.
    uint16_t SubsystemVendorID;
    uint16_t SubsystemID;
    uint32_t ExpansionROMBaseAddr;
    uint8_t CapabilitiesPointer;
    uint8_t Reserved0;
    uint16_t Reserved1;
    uint32_t Reserved2;
    uint8_t InterruptLine;
    uint8_t InterruptPin;
    uint8_t MinGrant;
    uint8_t MaxLatency;
} __attribute__((packed));

struct PCIBRIDGE_Header {
    PCI_Header pci;
    GenericBAR BAR0;
    GenericBAR BAR1;
    uint8_t PrimaryBusNum;
    uint8_t SecondaryBusNum;
    uint8_t SubordinateBusNum;
    uint8_t SecondaryLatencyTimer;
    uint8_t IOBase;
    uint8_t IOLimit;
    uint16_t SecondaryStatus;
    uint16_t MemoryBase;
    uint16_t MemoryLimit;
    uint16_t PrefetchableMemoryBase;
    uint16_t PrefetchableMemoryLimit;
    uint32_t PrefetchableBaseUpper32;
    uint32_t PrefetchableLimitUpper32;
    uint16_t IOBaseUpper16;
    uint16_t IOLimitUpper16;
    uint8_t CapabilityPointer;
    uint8_t Reserved0;
    uint16_t Reserved1;
    uint32_t ExpansionROMBaseAddr;
    uint8_t InterruptLine;
    uint8_t InterruptPin;
    uint16_t BridgeControl;
} __attribute__((packed));

struct PCICardBus_Header {
    PCI_Header pci;
    uint32_t CardBusBaseAddr;
    uint8_t OffsetCapabilitiesList;
    uint8_t Reserved;
    uint16_t SecondaryStatus;
    uint8_t PCIBusNum;
    uint8_t CardBusNum;
    uint8_t SubordinateBusNum;
    uint8_t CardBusLatencyTimer;
    MemSpaceBAR MemoryBaseAddr0;
    uint32_t MemoryLimit0;
    MemSpaceBAR MemoryBaseAddr1;
    uint32_t MemoryLimit1;
    IOSpaceBAR IOBaseAddr0;
    uint32_t IOLimit0;
    IOSpaceBAR IOBaseAddr1;
    uint32_t IOLimit1;
    uint8_t InterruptLine;
    uint8_t InterruptPin;
    uint16_t BridgeControl;
    uint16_t SubsystemDeviceID;
    uint16_t SubsystemVendorID;
    uint32_t _16BitPCCardLegacyModeBaseAddr;
} __attribute__((packed));

class PCI {
public:
    PCI() {}

    uint16_t ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    bool PCIExists();
};