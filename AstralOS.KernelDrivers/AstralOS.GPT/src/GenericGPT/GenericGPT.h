#pragma once
#define DRIVER
#include <new>
#include "../DriverServices.h"

class GenericGPTDeviceFactory : public PartitionDeviceFactory {
public:
    virtual ~GenericGPTDeviceFactory() {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual PartitionDevice* CreateDevice() override;
};

class GenericGPTDevice : public PartitionDevice {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override;
    virtual bool ReadSector(uint64_t lba, void* buffer) override;
    virtual bool WriteSector(uint64_t lba, void* buffer) override;

    virtual uint64_t SectorCount() const override;
    virtual uint32_t SectorSize() const override;

    virtual uint8_t GetClass() override;
    virtual uint8_t GetSubClass() override;
    virtual uint8_t GetProgIF() override;
    virtual uint8_t GetPartition() override;
    virtual const char* name() const override;
    virtual const char* DriverName() const override;
    virtual BlockDevice* GetParentLayer() override;
    virtual bool SetMount(uint64_t FSID) override;
    virtual bool SetMountNode(FsNode* Node) override;
    virtual uint64_t GetMount() override;
    virtual FsNode* GetMountNode() override;
private:
	PartitionController* pcdev;
	DriverServices* _ds = nullptr;
    DeviceKey devKey;
    uint8_t partition;
};