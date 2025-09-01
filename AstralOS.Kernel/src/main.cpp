#include "KernelUtils.h"
#include <initializer_list>
#include <cstddef>
#include "Utils/utils.h"

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

    kernelServices.basicConsole.Println(to_hstring(current_addr));
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.pBootInfo.initrdBase));
/*
    kernelServices.initram.Initialize(pBootInfo->initrdBase, pBootInfo->initrdSize);

    if (kernelServices.initram.file_exists((char*)"a.txt")) {
        kernelServices.basicConsole.Println("a.txt exists!");
    } else {
        kernelServices.basicConsole.Println("a.txt doesn't exist!");
    }
    if (kernelServices.initram.file_exists((char*)"b.txt")) {
        kernelServices.basicConsole.Println("b.txt exists!");
    } else {
        kernelServices.basicConsole.Println("b.txt doesn't exist!");
    }

    Array<char[512]> files = kernelServices.initram.list((char*)"");
    if (files.size() == 0) {
        kernelServices.basicConsole.Println("No files in root dir");
    }

    kernelServices.basicConsole.Println("Files in root: ");

    for (size_t i = 0; i < files.size(); ++i) {
        kernelServices.basicConsole.Println((const char*)files[i]);
    }*/
/*
    size_t size;
    char* content = (char*)kernelServices.initram.read("", "a.txt", &size);

    if (!content) {
        kernelServices.basicConsole.Println("Could not find a.txt");
    } else {
        kernelServices.basicConsole.Println("Contents of a.txt:");
        kernelServices.basicConsole.Println(content);
    }
*/
kernelServices.basicConsole.Print("hi");

    /*
     * Initialize our Heap.
    */
    kernelServices.heapAllocator.Initialize();

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

    kernelServices.pcie.InitializePCIe(kernelServices.acpi.GetMCFG());
    if (kernelServices.pcie.PCIeExists()) {
        kernelServices.basicConsole.Println("PCIe Exists");
        kernelServices.pcie.Initialize();
    } else if (kernelServices.pci.PCIExists()) {
        kernelServices.basicConsole.Println("PCI Exists.");
        kernelServices.pci.Initialize();
    } else {
        kernelServices.basicConsole.Println("No PCI/PCIe devices found.");
    }

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
    Array<uint64_t> test;
    Array<char*> test2;

    test.push_back(99);
    test.push_back(98);
    test2.push_back((char*)"hi");
    test.push_back(90);
    test.push_back(79);

    kernelServices.basicConsole.Println(to_string(test[0]));
    kernelServices.basicConsole.Println(to_string(test[1]));
    kernelServices.basicConsole.Println(to_string(test[2]));
    kernelServices.basicConsole.Println(test2[0]);

    kernelServices.basicConsole.Println("Array Size Test: ");
    
    for (size_t i = 0; i < test.size(); i++) {
        kernelServices.basicConsole.Println(to_string(test[i]));
    }
    

    while (true) {
        
    }
    return 0;
}