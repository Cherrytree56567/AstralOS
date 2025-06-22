#include "KernelUtils.h"

extern "C" int _start(BootInfo* pBootInfo) {
    /*
     * Disable Interrupts
    */
    asm volatile("cli" ::: "memory");
	KernelServices kernelServices(pBootInfo);

    InitializePaging(&kernelServices, pBootInfo);

    memset(pBootInfo->pFramebuffer->BaseAddress, 0, pBootInfo->pFramebuffer->BufferSize);
    kernelServices.basicConsole.Println("Framebuffer Cleared");

    kernelServices.pageTableManager.MapMemory((void*)0x600000000, (void*)0x80000);
	kernelServices.basicConsole.Println("Mapped 0x600000000 to 0x80000");

    uint64_t* test = (uint64_t*)0x600000000;
    *test = 26;

    kernelServices.basicConsole.Println(to_string(*test));

    kernelServices.gdt.Create64BitGDT();

    uint16_t cs;
    asm volatile ("mov %%cs, %0" : "=r" (cs));

    uint16_t ds;
    asm volatile ("mov %%ds, %0" : "=r" (ds));

    kernelServices.basicConsole.Print("GDT Reg: CS = ");
    kernelServices.basicConsole.Print(to_hstring(cs));
    kernelServices.basicConsole.Print(", DS = ");
    kernelServices.basicConsole.Println(to_hstring(ds));

    InitializeIDT(&kernelServices, pBootInfo);

    if (kernelServices.apic.CheckAPIC()) {
        kernelServices.basicConsole.Println("APIC is Supported.");
        kernelServices.apic.EnableAPIC();
        kernelServices.basicConsole.Println("APIC is Enabled.");
    }

    kernelServices.apic.WriteAPIC(APICRegs::ICR1, 0);
    kernelServices.apic.WriteAPIC(APICRegs::ICR0, 0x00004500);

    if (kernelServices.apic.IsInterruptPending()) {
        kernelServices.basicConsole.Println("APIC Interrupt is Pending.");
    }

    while (true) {
        
    }
    return 0;
}