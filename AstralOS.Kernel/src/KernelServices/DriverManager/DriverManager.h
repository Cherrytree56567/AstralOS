#pragma once
#include <cstdint>
#include "../../Utils/Array/Array.h"
#include "../PCI/PCI.h"
#include "../../Utils/cpu.h"
#include "../ELF/elf.h"
#include "../../Utils/String/String.h"
#include "DriverServices.h"

class DriverManager {
public:
    void Initialize();
    void RegisterDriver(BaseDriverFactory* factory);
    void DetectDevices(Array<DeviceKey>& devices);
    void DetectDrivers();
    BaseDriver* GetDevice(uint8_t _class, uint8_t subclass, uint8_t progIF);
    const Array<BaseDriver*>& GetDevices() const;
    void CreateDriverServices();
    DriverServices& GetDS();

private:
    Array<BaseDriverFactory*> factories;
    Array<BaseDriver*> DeviceDrivers;
    DriverServices ds;
};