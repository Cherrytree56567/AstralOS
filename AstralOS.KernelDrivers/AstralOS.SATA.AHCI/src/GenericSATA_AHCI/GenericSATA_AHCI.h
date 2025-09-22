#pragma once
#include "../PCI.h"
#include "../DriverServices.h"

class GenericSATA_AHCI : public BlockDeviceFactory {
public:
    virtual ~GenericSATA_AHCI() override {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual BlockDevice* CreateDevice() override;
};

class GenericSATA_AHCIDevice : public BlockDevice {
public:
    virtual void Init(DriverServices& ds, const DeviceKey& devKey) = 0;
    virtual bool ReadSector(uint64_t lba, void* buffer) = 0;
    virtual bool WriteSector(uint64_t lba, const void* buffer) = 0;

    virtual uint64_t SectorCount() const = 0;
    virtual uint32_t SectorSize() const = 0;
    virtual void* GetInternalBuffer() = 0;

    virtual uint8_t GetClass() override = 0;
    virtual uint8_t GetSubClass() override = 0;
    virtual uint8_t GetProgIF() override = 0;
    virtual const char* name() const override = 0;
    virtual const char* DriverName() const override = 0;
private:
    DriverServices* ds = nullptr;
    DeviceKey& devKey;
};