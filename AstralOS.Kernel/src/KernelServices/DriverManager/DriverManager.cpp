#define KERNEL
#include "DriverManager.h"
#include "../KernelServices.h"

/*
 * Ok, here is a quick exp of how this all works.
 * 
 * The initialize func will read all drivers from
 * the `Drivers` dir in the Initial Ram Disk. The
 * Driver will then register a Base Driver class.
 * 
 * Then we can run DetectDevices which checks all
 * PCI/PCIe Devices and assigns the Driver a PCIe
 * /PCI Device. Then it will initialize the Base-
 * Driver.
 * 
 * Ik this all looks complicated, but I will add
 * a str combiner func which combines str's.
*/

void DriverManager::Initialize() {
    CreateDriverServices();

    factories.clear();

    Array<char*> files = ks->initram.list((char*)"Drivers");
    if (files.size() == 0) {
        ks->basicConsole.Println("No drivers avaliable :(");
    }

    for (size_t i = 0; i < files.size(); ++i) {
        if (ks->initram.file_exists((char*)((String)"Drivers/" + files[i] + "/driver.elf").c_str())) {
            void* elf = ks->initram.read((char*)((String)"Drivers/" + files[i]).c_str(), (char*)"driver.elf");
            Elf64_Ehdr* hdr = GetELFHeader(elf);
            if (ValidateEhdr(hdr)) {
                //ks->basicConsole.Println("ELF Driver is Valid!");
            } else {
                ks->basicConsole.Println("ELF Driver is Invalid!");
                free(elf);
                break;
            }

            uint64_t entry = LoadElf(hdr);
            if (entry) {
                DriverInfo (*driver_entry)(DriverServices&) = (DriverInfo(*)(DriverServices&))entry;
                DriverInfo di = driver_entry(GetDS());
            }
        }
    }
}

void DriverManager::RegisterDriver(BaseDriverFactory* factory) {
    factories.push_back(factory);
}

void DriverManager::AddDriver(BaseDriver* drv) {
    DeviceDrivers.push_back(drv);
}

/*
 * We can use this func to detect devices
*/
void DriverManager::DetectDevices(Array<DeviceKey>& devices) {
    for (size_t i = 0; i < (devices.size()); i++) {
        DeviceKey dev = devices.get(i);
        for (size_t j = 0; j < factories.size(); j++) {
            auto& factory = factories[j];
            if ((factory)->Supports(dev)) {
                BaseDriver* device = factory->CreateDevice();
                device->Init(ds, dev);
                AddDriver(device);
                break;
            }
        }
    }
}

/*
 * Right now, our Driver Manager only works
 * for PCI/PCIe Devices. But if we want to
 * have drivers for Partitioning and VFS's
 * and stuff like that, we need to have a 
 * function that gives drivers direct contol
 * over the data of other drivers.
 * 
 * EG: Block Drivers
 *           |
 *           V
 *   Partition Drivers
 *           |
 *           V
 *      FS Drivers
 * 
 * From here everything gets really really
 * confusing. So you need to have a layer
 * system, where you have PCI/PCIe devices
 * with software layers on top (eg: an ahci
 * controller which manages drives would use
 * a driver for partitions which would use a
 * driver for filesystems).
 * 
 * One way to do this, is to give each layer
 * a function that points back to the last
 * layer.
*/
void DriverManager::DetectDrivers(size_t layer) {
    for (size_t i = 0; i < (DeviceDrivers.size()); i++) {
        BaseDriver* dev = DeviceDrivers.get(i);
        if (layer != 0) {
            if (dev->GetDriverType() != DriverType::PartitionDevice) continue;
            if (dev->GetDriverType() != DriverType::FilesystemDriver) continue;
        }
        DeviceKey devKey;
        devKey.bars[0] = ((uint64_t)dev >> 32); // HIGH: 0x12345678XXXXXXXX
        devKey.bars[1] = ((uint64_t)dev & 0xFFFFFFFF); // LOW: 0xXXXXXXXXABCDEF00
        devKey.bars[2] = 22;
        for (size_t j = 0; j < factories.size(); j++) {
            auto& factory = factories[j];
            if (factory->GetLayerType() != SOFTWARE) continue;
            if ((factory)->Supports(devKey)) {
                BaseDriver* device = factory->CreateDevice();
                device->Init(ds, devKey);
                AddDriver(device);
                if (layer == 0) {
                    break;
                }
            }
        }
    }
}

BaseDriver* DriverManager::GetDevice(uint8_t _class, uint8_t subclass, uint8_t progIF) {
    for (size_t i = 0; i < DeviceDrivers.size(); ++i) {
        auto& dev = DeviceDrivers[i];
        if (dev->GetClass() == _class && dev->GetSubClass() == subclass && dev->GetProgIF() == progIF) return dev;
    }
    return nullptr;
}

Array<BaseDriver*> DriverManager::GetDevices(DriverType::_DriverType drvT) {
    Array<BaseDriver*> arr;
    arr.clear();
    for (size_t i = 0; i < DeviceDrivers.size(); ++i) {
        auto& dev = DeviceDrivers[i];
        if (dev->GetDriverType() == drvT) arr.push_back(dev);
    }
    return arr;
}

const Array<BaseDriver*>& DriverManager::GetDevices() const {
    return DeviceDrivers;
}

/*
 * We can use lambda functions to 
 * create or pass funcs to the
 * DriverServices struct.
 * 
 * TODO: Add perms for drivers and stuff
*/
void DriverManager::CreateDriverServices() {
    /*
     * Debugging
    */
    ds.Print = [](const char* str) { 
        ks->basicConsole.Print(str);
    };

    ds.Println = [](const char* str) { 
        ks->basicConsole.Println(str);
    };

    /*
     * Memory
    */
    ds.RequestPage = []() { 
        return ks->pageFrameAllocator.RequestPage();
    };

    ds.LockPage = [](void* address) { 
        ks->pageFrameAllocator.LockPage(address);
    };

    ds.LockPages = [](void* address, uint64_t pageCount) { 
        ks->pageFrameAllocator.LockPages(address, pageCount);
    };

    ds.FreePage = [](void* address) { 
        ks->pageFrameAllocator.FreePage(address);
    };

    ds.FreePages = [](void* address, uint64_t pageCount) { 
        ks->pageFrameAllocator.FreePages(address, pageCount);
    };

    ds.MapMemory = [](void* virtualMemory, void* physicalMemory, bool cache) { 
        ks->pageTableManager.MapMemory(virtualMemory, physicalMemory, cache);
    };

    ds.UnMapMemory = [](void* virtualMemory) { 
        ks->pageTableManager.UnmapMemory(virtualMemory);
    };

    ds.malloc = [](size_t size) { 
        return ks->heapAllocator.malloc(size);
    };

    ds.free = [](void* ptr) { 
        return ks->heapAllocator.free(ptr);
    };

    ds.strdup = [](const char* str) { 
        return strdup(str);
    };

    /*
     * IRQs
    */
    ds.SetDescriptor = [](uint8_t vector, void* isr, uint8_t flags) { 
        ks->idt.SetDescriptor(vector, isr, flags);
    };

    /*
     * PCI/e
    */
    ds.PCIExists = []() { 
        return ks->pci.PCIExists();
    };

    ds.PCIeExists = []() { 
        return ks->pcie.PCIeExists();
    };

    ds.EnableMSI = [](uint8_t bus, uint8_t device, uint8_t function, uint8_t vector) { 
        return ks->pci.EnableMSI(bus, device, function, vector);
    };

    ds.EnableMSIx = [](uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector) { 
        return ks->pcie.EnableMSIx(segment, bus, device, function, vector);
    };

    ds.ConfigReadWorde = [](uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector) { 
        return ks->pcie.ConfigReadWord(segment, bus, device, function, vector);
    };

    ds.ConfigWriteWorde = [](uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector, uint16_t value) { 
        ks->pcie.ConfigWriteWord(segment, bus, device, function, vector, value);
    };

    ds.ConfigWriteDWorde = [](uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector, uint32_t value) { 
        ks->pcie.ConfigWriteDWord(segment, bus, device, function, vector, value);
    };

    ds.ConfigReadBytee = [](uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector) {
        return ks->pcie.ConfigReadByte(segment, bus, device, function, vector);
    };

    ds.ConfigReadDWorde = [](uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector) { 
        return ks->pcie.ConfigReadDWord(segment, bus, device, function, vector);
    };

    /*
     * Driver Manager
    */
    ds.RegisterDriver = [](BaseDriverFactory* factory) { 
        ks->driverMan.RegisterDriver(factory);
    };

    ds.AddDriver = [](BaseDriver* drv) {
        ks->driverMan.AddDriver(drv);
    };

    /*
     * Timer Stuff
    */
    ds.sleep = [](uint64_t ms) { 
        ks->timer.sleep(ms);
    };
}

DriverServices& DriverManager::GetDS() {
    return ds;
}