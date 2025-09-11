#pragma once
#define DRIVER
#include "../DriverServices.h"
#include "../DriverManager.h"

class GenericSATA_AHCI : public BlockDeviceFactory {
public:
    virtual ~GenericSATA_AHCI() override {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual BlockDevice* CreateDevice() override;
};