#pragma once
#include "../PCI/PCI.h"

struct PCIeDeviceKey {
    uint16_t Segment;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    bool hasMSIx;
};

class PCIe {
public:
    PCIe() {}

    void Initialize();
    bool PCIeExists();

    uint32_t GetDeviceClass(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF);
    char* GetClassCode(uint8_t ClassCode);
    char* GetDeviceCode(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF);

    void checkAllSegments();
    void checkBus(uint16_t segment, uint8_t bus);

    bool EnableMSI(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector);

    Array<PCIeDeviceKey> GetDevices();
private:
    Array<PCIeDeviceKey> Devices;

    uint16_t ConfigReadWord(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    void ConfigWriteWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
    void ConfigWriteDWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
    uint8_t ConfigReadByte(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t ConfigReadDWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    bool deviceAlreadyFound(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
    void addDevice(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, bool hasMSIs);
    
    void checkFunction(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
    void checkDevice(uint16_t segment, uint8_t bus, uint8_t device);
    uint16_t getVendorID(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
    uint8_t getHeaderType(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);
};