#include "DriverManager.h"

void DiskManager::RegisterDriver(BlockDeviceFactory* factory) {
    factories.push_back(factory);
}

void DiskManager::DetectDevices(Array<DeviceKey>& devices, KernelServices& kernel) {
    for (size_t i = 0; i < devices.size; ++i) {
        auto dev = devices[i];
        for (size_t j = 0; j < factories.size; ++j) {
            auto& factory = factories[j];
            if (factory->Supports(dev)) {
                BlockDevice* device = factory->CreateDevice();
                device->Init(dev, kernel);
                blockDevices.push_back(device);
                break;
            }
        }
    }
}

BlockDevice* DiskManager::GetDeviceByName(const char* name) {
    for (size_t i = 0; i < blockDevices.size; ++i) {
        auto& dev = blockDevices[i];
        if (strcmp(dev->DeviceName(), name) == 0) return dev;
    }
    return nullptr;
}

const Array<BlockDevice*>& DiskManager::GetDevices() const {
    return blockDevices;
}