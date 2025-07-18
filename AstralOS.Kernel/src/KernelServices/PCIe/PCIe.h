#pragma once
#include "../PCI/PCI.h"
#include "../ACPI/ACPI.h"

struct MSIXMCRegs {
    uint16_t TableSize : 11;
    uint16_t Reserved : 3;
    uint16_t FunctionMask : 1;
    uint16_t Enable : 1;
} __attribute__((packed));

struct MSIX {
    uint8_t CapabilityID;
    uint8_t NextPointer;
    uint16_t MessageControl;
    uint32_t Table;
    uint32_t PendingBit;

    uint8_t ReadBIR() const {
        return Table & 0x7;
    }

    void WriteBIR(uint8_t bir) {
        Table = (Table & ~0x7) | (bir & 0x7);
    }

    uint32_t ReadTableOffset() const {
        return Table >> 3;
    }

    void WriteTableOffset(uint32_t offset) {
        Table = (Table & 0x7) | (offset << 3);
    }

    uint8_t ReadPendingBitBIR() const {
        return PendingBit & 0x7;
    }

    void WritePendingBitBIR(uint8_t bir) {
        PendingBit = (PendingBit & ~0x7) | (bir & 0x7);
    }

    uint32_t ReadPendingBitOffset() const {
        return PendingBit >> 3;
    }

    void WritePendingBitOffset(uint32_t offset) {
        PendingBit = (PendingBit & 0x7) | (offset << 3);
    }
} __attribute__((packed));

class PCIe {
public:
    PCIe() {}

    void Initialize();
    void InitializePCIe(MCFG* mcfg);
    bool PCIeExists();

    uint32_t GetDeviceClass(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF);
    char* GetClassCode(uint8_t ClassCode);
    char* GetDeviceCode(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF);

    void checkAllSegments();
    void checkBus(uint16_t segment, uint8_t bus);

    bool EnableMSIx(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector);

    Array<DeviceKey> GetDevices();
private:
    Array<DeviceKey> Devices;
    MCFG* mcfgTable;
    int numSegments;

    volatile uint32_t* GetECAMBase(uint16_t segment);

    uint16_t ConfigReadWord(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    void ConfigWriteWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
    void ConfigWriteDWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
    uint8_t ConfigReadByte(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t ConfigReadDWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    bool deviceAlreadyFound(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
    void addDevice(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, bool hasMSIx, uint16_t vendorID, uint8_t classCode, uint8_t subClass, uint8_t progIF);
    
    void checkFunction(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
    void checkDevice(uint16_t segment, uint8_t bus, uint8_t device);
    uint16_t getVendorID(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
    uint8_t getHeaderType(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
};