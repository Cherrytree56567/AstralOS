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
        kernelServices.ioapic.Initialize(&kernelServices.basicConsole, 0xFEC00000, 0, 0);
        kernelServices.basicConsole.Println("APIC is Enabled.");
    }

    uint32_t eax, edx;
    kernelServices.apic.ReadMSR(0x1B, &eax, &edx); // IA32_APIC_BASE_MSR
    kernelServices.basicConsole.Print("MSR EAX = ");
    kernelServices.basicConsole.Println(to_hstring(eax));
    kernelServices.basicConsole.Print("MSR EDX = ");
    kernelServices.basicConsole.Println(to_hstring(edx));

    kernelServices.basicConsole.Print("LAPIC ID = ");
    uint32_t lapicIdRaw = kernelServices.apic.ReadAPIC(APICRegs::lapicID);
    kernelServices.basicConsole.Println(to_hstring(lapicIdRaw));

    uint32_t eaxa, ebxa, ecxa, edxa;
    cpuid(1, &eaxa, &ebxa);
    uint8_t cpuApicId = (ebxa >> 24) & 0xFF;

    kernelServices.basicConsole.Println(to_hstring(cpuApicId));

    asm volatile("cli" ::: "memory");

    // Vector 0x40 (64), periodic mode (bit 17 = 1)
    kernelServices.apic.WriteAPIC(APICRegs::lvtTimer, 0x40 | 0x20000);

    // Initial timer count (how fast it counts down)
    kernelServices.apic.WriteAPIC(APICRegs::ICT, 0x100000);

    __asm__ volatile ("sti");

    kernelServices.basicConsole.Println("Checking APIC Interrupts.");
    if (kernelServices.apic.IsInterruptPending()) {
        kernelServices.basicConsole.Println("APIC Interrupt is Pending.");
    }

    while (true) {
        
    }
    return 0;
}