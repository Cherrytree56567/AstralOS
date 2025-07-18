#include "KernelUtils.h"
#include <initializer_list>

/*
 * Want to Learn OSDev
 * Have a look at the code to understand
 * how the kernel works.
 * Read the comments.
*/
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

    kernelServices.initram.Initialize(pBootInfo->initrdBase, pBootInfo->initrdSize);

    char* files = kernelServices.initram.list((char*)"");
    if (!files) {
        kernelServices.basicConsole.Println("No files in root dir");
    }

    kernelServices.basicConsole.Println("Files in root: ");

    char* current = files;
    while (*current) {
        kernelServices.basicConsole.Print(" - ");
        kernelServices.basicConsole.Println(current);
        size_t len = 0;
        while (current[len] != '\0') {
            ++len;
        }
        current += len + 1;
    }

    size_t size;
    char* content = (char*)kernelServices.initram.read("", "a.txt", &size);

    if (!content) {
        kernelServices.basicConsole.Println("Could not find a.txt");
    } else {
        kernelServices.basicConsole.Println("Contents of a.txt:");
        kernelServices.basicConsole.Println(content);
    }

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

    while (true) {
        
    }
    return 0;
}