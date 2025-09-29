#pragma once
#define DRIVER
#include "../PCI.h"
#include "../DriverServices.h"
#include <cstdint>

class GenericGPT : public PartitionDriverFactory {
public:
    virtual ~GenericGPT() {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual PartitionDevice* CreateDevice() override;
};

class GenericGPTDevice : public PartitionDevice {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override;
    virtual bool ReadSector(uint64_t lba, void* buffer);
    virtual bool WriteSector(uint64_t lba, void* buffer);
    virtual bool SetParition(uint8_t partition);

    virtual uint64_t SectorCount() const;
    virtual uint32_t SectorSize() const;

    virtual uint8_t GetClass() override;
    virtual uint8_t GetSubClass() override;
    virtual uint8_t GetProgIF() override;
    virtual const char* name() const override;
    virtual const char* DriverName() const override;
    virtual BaseDriver* GetParentLayer() override;
private:
	BlockDevice* bldev;
	DriverServices* _ds = nullptr;
    DeviceKey devKey;
};