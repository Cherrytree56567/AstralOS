#pragma once
#include "../PCI.h"
#include "../DriverServices.h"

class GenericSATA_AHCI : public BlockDeviceFactory {
public:
    virtual ~GenericSATA_AHCI() override {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual BlockDevice* CreateDevice() override;
};