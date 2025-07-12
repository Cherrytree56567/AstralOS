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

	KernelServices kernelServices(pBootInfo);

    kernelServices.pBootInfo = *pBootInfo;

    InitializePaging(&kernelServices, pBootInfo);

    uint64_t ptr = (uint64_t)&start;
    uintptr_t virt_ptr = (ptr - 0x1000) + HIGHER_VIRT_ADDR;

    auto typed_func = (void(*)(KernelServices&, BootInfo*))virt_ptr;
    typed_func(kernelServices, &kernelServices.pBootInfo);
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
     * Use our own *Higher Half* stack
    */
    uint64_t stackVirtAddr = (uint64_t)&_stack_end + HIGHER_VIRT_ADDR;
    uint64_t stack_start = (uint64_t)&_stack_start;
    uint64_t stack_end   = (uint64_t)&_stack_end;
    uint64_t stack_size  = stack_end - stack_start;
    uint64_t stack_pages = (stack_size + 0xFFF) / 0x1000;
    //asm volatile ("mov %0, %%rsp" :: "r"(stackVirtAddr));

    kernelServices.basicConsole.pFramebuffer.BaseAddress = pBootInfo->pFramebuffer.BaseAddress;

    /*
     * Here we can unmap the memory
     * to know which parts of our kernel
     * are still using the lower mapped part.
    */
    for (uint64_t i = 0; i < stack_pages; i++) {
        //kernelServices.pageTableManager.UnmapMemory((void*)(stack_start + i * 0x1000));
    }

    /*
     * Check if we are indeed at the higher half address.
    */
    uint64_t current_addr;
    __asm__ volatile ("lea (%%rip), %0" : "=r"(current_addr));

    /*
     * Clear the framebuffer
    */
    memsetC(kernelServices.pBootInfo.pFramebuffer.BaseAddress, 0, kernelServices.pBootInfo.pFramebuffer.BufferSize);

    kernelServices.basicConsole.Println(to_hstring(current_addr));
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.pBootInfo.initrdBase));

    kernelServices.heapAllocator.Initialize();

    kernelServices.gdt.Create64BitGDT();

    kernelServices.acpi.Initialize(&kernelServices.basicConsole, kernelServices.pBootInfo.rsdp);

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