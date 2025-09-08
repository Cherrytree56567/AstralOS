#pragma once
#include <cstdint>

struct DeviceKey {
    bool PCIe;
    uint16_t segment; // ONLY FOR PCIe
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    bool hasMSIx;
    bool hasMSI;

    uint8_t classCode;
    uint8_t subclass;
    uint8_t progIF;

    uint16_t vendorID;

    uint32_t bars[6];
};