#include "DriverManager.h"

void DriverManager::RegisterDriver(BaseDriverFactory* factory) {
    factories.push_back(factory);
}

void DriverManager::DetectDevices(Array<DeviceKey>& devices, KernelServices& kernel) {
    for (size_t i = 0; i < devices.size; ++i) {
        auto dev = devices[i];
        for (size_t j = 0; j < factories.size; ++j) {
            auto& factory = factories[j];
            if (factory->Supports(dev)) {
                BaseDriver* device = factory->CreateDevice();
                device->Init(kernel);
                DeviceDrivers.push_back(device);
                break;
            }
        }
    }
}

BaseDriver* DriverManager::GetDevice(uint8_t _class, uint8_t subclass, uint8_t progIF) {
    for (size_t i = 0; i < DeviceDrivers.size; ++i) {
        auto& dev = DeviceDrivers[i];
        if (dev->GetClass() == _class && dev->GetSubClass() == subclass && dev->GetProgIF() == progIF) return dev;
    }
    return nullptr;
}

const Array<BaseDriver*>& DriverManager::GetDevices() const {
    return DeviceDrivers;
}