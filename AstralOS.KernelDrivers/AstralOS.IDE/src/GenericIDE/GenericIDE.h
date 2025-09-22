#pragma once
#include "../PCI.h"
#include "../DriverServices.h"

class GenericIDE : public BlockDeviceFactory {
public:
    virtual ~GenericIDE() override {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual BlockDevice* CreateDevice() override;
};

class GenericIDEDevice : public BlockDevice {
public:
    virtual void Init(DriverServices& ds) = 0;
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
};