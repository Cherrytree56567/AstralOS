#define KERNEL
#include "KernelUtils.h"
#include <initializer_list>
#include <cstddef>
#include "Utils/utils.h"
#include "KernelServices/ELF/elf.h"

/*
 * Want to Learn OSDev
 * Have a look at the code to understand
 * how the kernel works.
 * Read the comments.
*/
extern "C" void irq1_handler() {
    uint8_t scancode = inb(0x60);
    ks->basicConsole.Println("Key press!");
    ks->apic.WriteAPIC(APICRegs::EOI, 0);
}

extern "C" void irq_stub();

void Printks(const char* str) {
    ks->basicConsole.Println(str);
}

/*
 * Yep GPT Based
*/
void list_recursive(char* dir, int depth = 0) {
    Array<char*> files = ks->initram.list(dir);

    for (size_t i = 0; i < files.size(); ++i) {
        for (int d = 0; d < depth; d++) {
            ks->basicConsole.Print("   ");
        }

        ks->basicConsole.Print("- ");
        ks->basicConsole.Println(files[i]);

        char fullPath[512];
        fullPath[0] = '\0';

        if (strlen(dir) > 0) {
            strcpy(fullPath, dir);
            strcat(fullPath, "/");
            strcat(fullPath, files[i]);
        } else {
            strcpy(fullPath, files[i]);
        }

        if (ks->initram.dir_exists(fullPath)) {
            list_recursive(fullPath, depth + 1);
        }
    }
}

extern "C" int _start(BootInfo* pBootInfo) {
    /*
     * Disable Interrupts
     * If you don't, you WILL get weird errors.
    */
    asm volatile("cli" ::: "memory");

    /*
     * Use our own stack
    */
    asm volatile ("mov %0, %%rsp" :: "r"(&_stack_end));
    uint64_t stackVirtAddr = ((uint64_t)&_stack_end - 0x1000) + HIGHER_VIRT_ADDR;
    uint64_t ptr = (uint64_t)&start;
    uintptr_t virt_ptr = (ptr - 0x1000) + HIGHER_VIRT_ADDR;
    auto startFunc = reinterpret_cast<void (*)(KernelServices&, BootInfo*)>(virt_ptr);

	KernelServices kernelServices(pBootInfo);

    kernelServices.pBootInfo = *pBootInfo;

    ks->basicConsole.Print("Boot Info Framebuffer Addr: ");

    InitializePaging(&kernelServices, pBootInfo);

    asm volatile (
        "mov %0, %%rsp\n"
        "jmp *%1\n"
        :
        : "r"((void*)stackVirtAddr), "r"(startFunc), "D"(&kernelServices), "S"(&kernelServices.pBootInfo)
        : "memory"
    );
}
/*
 * We have separated these into 2, 
 * to implement the Higher Half Kernel.
 * The first part, initializes paging,
 * and then normalizes the `start` func
 * into a virtual address.
*/
extern "C" int start(KernelServices& kernelServices, BootInfo* pBootInfo) {
    /*
     * Check if we are indeed at the higher half address.
    */
    uint64_t current_addr;
    __asm__ volatile ("lea (%%rip), %0" : "=r"(current_addr));

    kernelServices.basicConsole.pFramebuffer.BaseAddress = pBootInfo->pFramebuffer.BaseAddress;

    /*
     * Clear the framebuffer
    */
    memsetC(kernelServices.pBootInfo.pFramebuffer.BaseAddress, 0, kernelServices.pBootInfo.pFramebuffer.BufferSize);

    kernelServices.basicConsole.Print("Current Addr: ");
    kernelServices.basicConsole.Println(to_hstring(current_addr));

    /*
     * Initialize our Heap.
    */
    kernelServices.heapAllocator.Initialize();

    /*
     * Initialize our Init RAM FS
     * after initializing our heap
     * because it requires heap.
    */
    kernelServices.initram.Initialize(pBootInfo->initrdBase, pBootInfo->initrdSize);
/*
    if (kernelServices.initram.file_exists((char*)"a.txt")) {
        kernelServices.basicConsole.Print("a.txt exists!");
    } else {
        kernelServices.basicConsole.Print("a.txt doesn't exist!");
    }
    if (kernelServices.initram.file_exists((char*)"b.txt")) {
        kernelServices.basicConsole.Println(", b.txt exists!");
    } else {
        kernelServices.basicConsole.Println(", b.txt doesn't exist!");
    }

    list_recursive((char*)"");*/

    /*
     * Create our GDT.
    */
    kernelServices.gdt.Create64BitGDT();

    /*
     * Initialize the ACPI.
    */
    kernelServices.acpi.Initialize(&kernelServices.basicConsole, kernelServices.pBootInfo.rsdp);

    /*
     * Initialize the IDT
     * Create the APIC
     * Initialize the I/O APIC
    */
    InitializeIDT(&kernelServices, pBootInfo);

    /*
     * Test ACPI
    */
    kernelServices.basicConsole.Print("MADT Address: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.acpi.GetMADT()));
    kernelServices.basicConsole.Print("MADT Processor Local APIC: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.acpi.GetProcessorLocalAPIC()));
    kernelServices.basicConsole.Print("MADT I/O APIC: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.acpi.GetIOAPIC()));

    /*
     * Heap Alloc test written using GPT.
    */
    void* ptr1 = kernelServices.heapAllocator.malloc(32);
    kernelServices.basicConsole.Print("Allocated 32 bytes at ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)ptr1));

    void* ptr2 = kernelServices.heapAllocator.malloc(64);
    kernelServices.basicConsole.Println("Allocated 64 bytes at ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)ptr2));

    memsetC(ptr1, 0xAA, 32);
    memsetC(ptr2, 0x55, 64);

    kernelServices.heapAllocator.free(ptr1);
    kernelServices.basicConsole.Println("Freed 32-byte block");

    void* ptr3 = kernelServices.heapAllocator.malloc(16);
    kernelServices.basicConsole.Print("Allocated 16 bytes at ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)ptr3));

    kernelServices.heapAllocator.free(ptr2);
    kernelServices.heapAllocator.free(ptr3);
    kernelServices.basicConsole.Println("Freed remaining blocks");

    /*
     * Test I/O APIC
    */

    RedirectionEntry entry = {};
    
    entry.vector = 0x21;         // vector = your IDT index
    entry.delvMode = 0;          // fixed
    entry.destMode = 0;          // physical
    entry.mask = 0;              // ENABLED
    entry.triggerMode = 0;       // edge-triggered for IRQ1
    entry.pinPolarity = 0;       // high active
    entry.destination = 0;       // LAPIC ID of current CPU
    
    kernelServices.ioapic.writeRedirEntry(1, &entry); // IRQ1 = GSI 1
    kernelServices.idt.SetDescriptor(0x21, (void*)irq_stub, 0x8E); // Set IDT entry for IRQ1

    /*
     * Test Array
    */
    kernelServices.basicConsole.Println("Array Size Test: ");
    Array<uint64_t> test;
    Array<char*> test2;

    test.push_back(99);
    test.push_back(98);
    test2.push_back((char*)"hi");
    test.push_back(90);
    test.push_back(79);

    kernelServices.basicConsole.Print(to_string(test[0]));
    kernelServices.basicConsole.Print(", ");
    kernelServices.basicConsole.Print(to_string(test[1]));
    kernelServices.basicConsole.Print(", ");
    kernelServices.basicConsole.Print(to_string(test[2]));
    kernelServices.basicConsole.Print(", ");
    kernelServices.basicConsole.Println(test2[0]);

    /*
     * Do PCI/PCIe Stuff
    */
    Array<DeviceKey> devices;

    kernelServices.pcie.InitializePCIe(kernelServices.acpi.GetMCFG());
    if (kernelServices.pcie.PCIeExists()) {
        kernelServices.basicConsole.Println("PCIe Exists");
        kernelServices.pcie.Initialize();

        devices = kernelServices.pcie.GetDevices();
    } else if (kernelServices.pci.PCIExists()) {
        kernelServices.basicConsole.Println("PCI Exists.");
        kernelServices.pci.Initialize();

        devices = kernelServices.pci.GetDevices();
    } else {
        kernelServices.basicConsole.Println("No PCI/PCIe devices found.");
    }

    kernelServices.timer.Calibrate();

    kernelServices.basicConsole.Print("Waiting for 1s...");
    kernelServices.timer.sleep(1000);
    kernelServices.basicConsole.Println("Done!");

    /*
     * Driver Stuff
    */
    kernelServices.driverMan.Initialize();
    kernelServices.driverMan.DetectDevices(devices);
    for (size_t i = 0; i < 2; i++) {
        kernelServices.driverMan.DetectDrivers(i);
    }

    /*
     * AHCI Driver Test
    */
    BaseDriver* driver = kernelServices.driverMan.GetDevice(0x1, 0x6, 0x1);
    if (driver) {
        kernelServices.basicConsole.Println(((String)"Found Driver: " + driver->DriverName()).c_str());
        BlockDevice* bldev = static_cast<BlockDevice*>(driver);
        for (int i = 0; i < 32; i++) {
            if (bldev->SetDrive(i) && bldev->SectorCount() > 0) {
                ks->basicConsole.Println(((String)"Drive " + (String)to_hstring((uint64_t)i) + " is present").c_str());
                break;
            } else {
                ks->basicConsole.Println(((String)"Drive " + (String)to_hstring((uint64_t)i) + " is not present").c_str());
            }
        }

        uint32_t sectorSize = bldev->SectorSize();
        uint32_t sectorCount = bldev->SectorCount();
        const char* model = bldev->name();

        ks->basicConsole.Println(((String)"Model: " + (String)model).c_str());
        ks->basicConsole.Println(((String)"Sector Size: " + (String)to_string((uint64_t)sectorSize)).c_str());
        ks->basicConsole.Println(((String)"Sector Count: " + (String)to_string((uint64_t)sectorCount)).c_str());

        uint64_t buf_phys = (uint64_t)ks->pageFrameAllocator.RequestPage();
        uint64_t buf_virt = 0xFFFFFFFF00000000 + buf_phys;

        ks->pageTableManager.MapMemory((void*)buf_virt, (void*)buf_phys, false);

        char* buffer = (char*)buf_virt;

        for (uint64_t lba = 5042; lba < 5047; lba++) {
            memset((void*)buf_virt, 0, sectorSize);
            if (bldev->ReadSector(lba, (void*)buf_phys))  {
                ks->basicConsole.Print(((String)"Drive Buffer LBA" + to_string(lba) + ": ").c_str());
                buffer[sectorSize - 1] = '\0';
                ks->basicConsole.Println((char*)buffer);
            } else {
                ks->basicConsole.Println("Failed Reading Sector");
            }
        }

        /*
         * We should reuse our old buffer!
        */
        memset((void*)buf_virt, 0, sectorSize);
        strcpy((char*)buf_virt, "Hello World!");

        /*
         * WARNING: THIS IS A DESTRUCTIVE TEST AND
         * CAN/WILL DESTROY DATA ON THE DISK.
         * 
         * THIS IS ONLY FOR DEBUGGING!!!
         * 
         * In this case, when you re-run the qemu
         * command (not the testing command, bc it
         * re copies the qcow2 file), it will show
         * `Hello Wo.rld` instead of the boot dir.
        */
        /*
        if (!bldev->WriteSector(5043, (void*)buf_phys))  {
            ks->basicConsole.Println("Failed Writing Sector");
        }*/

        for (uint64_t lba = 5042; lba < 5045; lba++) {
            memset((void*)buf_virt, 0, sectorSize);
            if (bldev->ReadSector(lba, (void*)buf_phys))  {
                ks->basicConsole.Print(((String)"Drive Buffer LBA" + to_string(lba) + ": ").c_str());
                buffer[sectorSize - 1] = '\0';
                ks->basicConsole.Println((char*)buffer);
            } else {
                ks->basicConsole.Println("Failed Reading Sector");
            }
        }
    }

    /*
     * GPT Driver Test
    */
    BaseDriver* Partdriver = kernelServices.driverMan.GetDevice(DriverType::Partition);
    if (Partdriver) {
        kernelServices.basicConsole.Println(((String)"Found Partition Driver: " + Partdriver->DriverName()).c_str());
        PartitionDevice* bldev = static_cast<PartitionDevice*>(Partdriver);

        uint64_t buf_phys = (uint64_t)ks->pageFrameAllocator.RequestPage();
        uint64_t buf_virt = 0xFFFFFFFF00000000 + buf_phys;

        ks->pageTableManager.MapMemory((void*)buf_virt, (void*)buf_phys, false);

        uint32_t sectorSize = bldev->SectorSize();
        uint32_t sectorCount = bldev->SectorCount();
        const char* model = bldev->name();

        ks->basicConsole.Println(((String)"Model: " + (String)model).c_str());
        ks->basicConsole.Println(((String)"Sector Size: " + (String)to_string((uint64_t)sectorSize)).c_str());
        ks->basicConsole.Println(((String)"Sector Count: " + (String)to_string((uint64_t)sectorCount)).c_str());

        char* buffer = (char*)buf_virt;

        for (uint64_t lba = 0; lba < 5; lba++) {
            memset((void*)buf_virt, 0, sectorSize);
            if (bldev->ReadSector(lba, (void*)buf_phys))  {
                ks->basicConsole.Print(((String)"Partition Buffer LBA" + to_string(lba) + ": ").c_str());
                buffer[sectorSize - 1] = '\0';
                ks->basicConsole.Println((char*)buffer);
            } else {
                ks->basicConsole.Println("Failed Reading Sector");
            }
        }
    }

    BaseDriver* FSDriver = kernelServices.driverMan.GetDevice(DriverType::Filesystem);
    if (FSDriver) {
        kernelServices.basicConsole.Println(((String)"Found FS Driver: " + FSDriver->DriverName()).c_str());
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver);

        for (size_t i = 0; i < 3; i++) {
            if (!bldev->GetParentLayer()->SetPartition(i)) continue;
            if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;
            if (bldev->Support()) {
                FsNode* fsN = bldev->Mount();

                size_t count = 0;
                FsNode* TestDir = bldev->CreateDir(fsN, "TestDir");
                FsNode** nodes = bldev->ListDir(fsN, &count);

                /*
                 * I couldn't be bothered to make a whole
                 * for loop to print everything, so I just
                 * asked GPT.
                */
                for (size_t i = 0; i < count; i++) {
                    FsNode* node = nodes[i];
                    if (!node) continue;

                    kernelServices.basicConsole.Print("Node ID: ");
                    kernelServices.basicConsole.Println(to_hstring(node->nodeId));

                    kernelServices.basicConsole.Print("Name: ");
                    kernelServices.basicConsole.Println(node->name ? node->name : "(null)");

                    kernelServices.basicConsole.Print("Type: ");
                    switch (node->type) {
                        case FsNodeType::File: kernelServices.basicConsole.Println("File"); break;
                        case FsNodeType::Directory: kernelServices.basicConsole.Println("Directory"); break;
                        default: kernelServices.basicConsole.Println("Unknown"); break;
                    }

                    kernelServices.basicConsole.Print("Size: ");
                    kernelServices.basicConsole.Println(to_hstring(node->size));

                    kernelServices.basicConsole.Print("Blocks: ");
                    kernelServices.basicConsole.Println(to_hstring(node->blocks));

                    kernelServices.basicConsole.Print("Mode: ");
                    kernelServices.basicConsole.Println(to_hstring(node->mode));

                    kernelServices.basicConsole.Print("UID: ");
                    kernelServices.basicConsole.Println(to_hstring(node->uid));

                    kernelServices.basicConsole.Print("GID: ");
                    kernelServices.basicConsole.Println(to_hstring(node->gid));

                    kernelServices.basicConsole.Print("ATime: ");
                    kernelServices.basicConsole.Println(to_hstring(node->atime));

                    kernelServices.basicConsole.Print("MTime: ");
                    kernelServices.basicConsole.Println(to_hstring(node->mtime));

                    kernelServices.basicConsole.Print("CTime: ");
                    kernelServices.basicConsole.Println(to_hstring(node->ctime));

                    kernelServices.basicConsole.Println("-------------------------");
                }

                FsNode* fself = bldev->FindDir(fsN, "kernel.elf");
                if (fself) {
                    kernelServices.basicConsole.Print("Found: ");
                    kernelServices.basicConsole.Println(fself->name);
                } else {
                    kernelServices.basicConsole.Println("Not Found: kernel.elf");
                }
            }
        }
        
    }

    while (true) {
        
    }
    return 0;
}