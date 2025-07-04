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
};

class PCI {
public:
    PCI() {}

    uint16_t ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    bool PCIExists();
};