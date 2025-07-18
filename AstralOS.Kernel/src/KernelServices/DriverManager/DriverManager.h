#pragma once
#include <cstdint>
#include "../Array/Array.h"
#include "../PCI/PCI.h"
#include "../KernelServices.h"
#include "../CPUutils/cpuid.h"

class BaseDriver {
public:
    virtual ~BaseDriver() {}
    virtual const char* name() const = 0;
    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual uint8_t GetClass() = 0;
    virtual uint8_t GetSubClass() = 0;
    virtual uint8_t GetProgIF() = 0;
    virtual uint8_t GetFSHeader() = 0;
};

class BlockDevice : public BaseDriver {
public:
    virtual void Init(const DeviceKey& devKey, KernelServices& kernServ) = 0;
    virtual bool ReadSector(uint64_t lba, void* buffer) = 0;
    virtual bool WriteSector(uint64_t lba, const void* buffer) = 0;

    virtual const char* DeviceName() const = 0;
    virtual uint64_t SectorCount() const = 0;
    virtual uint32_t SectorSize() const = 0;
    virtual void* GetInternalBuffer() = 0;

    virtual uint8_t GetClass() override = 0;
    virtual uint8_t GetSubClass() override = 0;
    virtual uint8_t GetProgIF() override = 0;
};

class BlockDeviceFactory {
public:
    virtual ~BlockDeviceFactory() {}
    virtual bool Supports(const DeviceKey& devKey) = 0;
    virtual BlockDevice* CreateDevice() = 0;
};

class DiskManager {
public:
    void RegisterDriver(BlockDeviceFactory* factory);
    void DetectDevices(Array<DeviceKey>& devices, KernelServices& kernel);
    BlockDevice* GetDeviceByName(const char* name);
    const Array<BlockDevice*>& GetDevices() const;

private:
    Array<BlockDeviceFactory*> factories;
    Array<BlockDevice*> blockDevices;
};