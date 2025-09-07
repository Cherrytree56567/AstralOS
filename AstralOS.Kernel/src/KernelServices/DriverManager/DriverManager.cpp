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

    Array<char*> files = ks->initram.list((char*)"Drivers");
    if (files.size() == 0) {
        ks->basicConsole.Println("No drivers avaliable :(");
    }

    for (size_t i = 0; i < files.size(); ++i) {
        /*
         * Janky af combiner
        */
        char fullPath[720];
        char fullPatha[800];
        strcpy(fullPath, "Drivers/");
        strcat(fullPath, files[i]);
        strcpy(fullPatha, fullPath);
        strcat(fullPatha, "/driver.elf");
        if (ks->initram.file_exists(fullPatha)) {
            void* elf = ks->initram.read(fullPath, (char*)"driver.elf");
            Elf64_Ehdr* hdr = GetELFHeader(elf);
            if (ValidateEhdr(hdr)) {
                ks->basicConsole.Println("ELF Driver is Valid!");
            } else {
                ks->basicConsole.Println("ELF Driver is Invalid!");
                free(elf);
                break;
            }

            uint64_t entry = LoadElf(hdr);
            if (entry) {
                DriverInfo (*driver_entry)(DriverServices) = (DriverInfo(*)(DriverServices))entry;
                DriverInfo di = driver_entry(GetDS());
                ks->basicConsole.Print("Driver Name: ");
                ks->basicConsole.Print(di.name);
                ks->basicConsole.Print(", Version: ");
                ks->basicConsole.Print(to_string((int64_t)di.verMaj));
                ks->basicConsole.Print(".");
                ks->basicConsole.Print(to_string((int64_t)di.verMin));
                ks->basicConsole.Print(", Exit Code: ");
                ks->basicConsole.Println(to_string((int64_t)di.exCode));
            }
        }
    }
}

void DriverManager::RegisterDriver(BaseDriverFactory* factory) {
    factories.push_back(factory);
}

/*
 * We can use this func to detect devices
*/
void DriverManager::DetectDevices(Array<DeviceKey>& devices) {
    for (size_t i = 0; i < devices.size(); ++i) {
        auto dev = devices[i];
        for (size_t j = 0; j < factories.size(); ++j) {
            auto& factory = factories[j];
            if (factory->Supports(dev)) {
                BaseDriver* device = factory->CreateDevice();
                device->Init(ds);
                DeviceDrivers.push_back(device);
                break;
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

const Array<BaseDriver*>& DriverManager::GetDevices() const {
    return DeviceDrivers;
}

/*
 * We can use lambda functions to 
 * create or pass funcs to the
 * DriverServices struct.
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

    ds.MapMemory = [](void* virtualMemory, void* physicalMemory) { 
        ks->pageTableManager.MapMemory(virtualMemory, physicalMemory);
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
}

DriverServices DriverManager::GetDS() {
    return ds;
}