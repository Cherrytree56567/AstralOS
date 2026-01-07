#pragma once
#define DRIVER
#include <new>
#include "../global.h"

class GenericIDEFactory : public BlockDeviceFactory {
public:
    virtual ~GenericIDEFactory() override {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual BlockDevice* CreateDevice() override;
};

class GenericIDE : public BlockDevice {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override;
    virtual bool ReadSector(uint64_t lba, void* buffer) override;
    virtual bool WriteSector(uint64_t lba, void* buffer) override;

    virtual uint64_t SectorCount() const override;
    virtual uint32_t SectorSize() const override;
    virtual void* GetInternalBuffer() override;
    virtual const char* name() const override;

    virtual uint8_t GetClass() override;
    virtual uint8_t GetSubClass() override;
    virtual uint8_t GetProgIF() override;
    virtual uint8_t GetDrive() override;
    virtual const char* DriverName() const override;
    virtual BlockController* GetParentLayer() override;
private:
    DriverServices* _ds = nullptr;
    DeviceKey devKey;
    uint8_t drive;
    BlockController* pdev;
};