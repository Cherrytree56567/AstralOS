#pragma once
#include <cstdint>
#include "../../Utils/Array/Array.h"
#include "../PCI/PCI.h"
#include "../KernelServices.h"
#include "../../Utils/cpu.h"

class BaseDriver {
public:
    virtual ~BaseDriver() {}
    virtual const char* name() const = 0;
    virtual const char* DriverName() const = 0;
    virtual void Init(KernelServices& kernServ) = 0;
    virtual uint8_t GetClass() = 0;
    virtual uint8_t GetSubClass() = 0;
    virtual uint8_t GetProgIF() = 0;
};

class BaseDriverFactory {
public:
    virtual ~BaseDriverFactory() {}
    virtual bool Supports(const DeviceKey& devKey) = 0;
    virtual BaseDriver* CreateDevice() = 0;
};

class BlockDevice : public BaseDriver {
public:
    virtual void Init(KernelServices& kernServ) = 0;
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

class BlockDeviceFactory : BaseDriverFactory {
public:
    virtual ~BlockDeviceFactory() {}
    virtual bool Supports(const DeviceKey& devKey) override = 0;
    virtual BlockDevice* CreateDevice() override = 0;
};

class DriverManager {
public:
    void RegisterDriver(BaseDriverFactory* factory);
    void DetectDevices(Array<DeviceKey>& devices, KernelServices& kernel);
    BaseDriver* GetDevice(uint8_t _class, uint8_t subclass, uint8_t progIF);
    const Array<BaseDriver*>& GetDevices() const;

private:
    Array<BaseDriverFactory*> factories;
    Array<BaseDriver*> DeviceDrivers;
};