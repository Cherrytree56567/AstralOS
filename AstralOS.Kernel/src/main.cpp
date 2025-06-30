#include "KernelUtils.h"

extern "C" void irq1_handler() {
    uint8_t scancode = inb(0x60);
    ks->basicConsole.Println("Key press!");

    ks->apic.WriteAPIC(APICRegs::EOI, 0);
}

extern "C" void irq_stub();

extern "C" int _start(BootInfo* pBootInfo) {
    /*
     * Disable Interrupts
    */
    asm volatile("cli" ::: "memory");
	KernelServices kernelServices(pBootInfo);

    InitializePaging(&kernelServices, pBootInfo);

    memsetC(pBootInfo->pFramebuffer->BaseAddress, 0, pBootInfo->pFramebuffer->BufferSize);
    kernelServices.basicConsole.Println("Framebuffer Cleared");

    kernelServices.gdt.Create64BitGDT();

    uint16_t cs;
    asm volatile ("mov %%cs, %0" : "=r" (cs));

    uint16_t ds;
    asm volatile ("mov %%ds, %0" : "=r" (ds));

    kernelServices.basicConsole.Print("GDT Reg: CS = ");
    kernelServices.basicConsole.Print(to_hstring(cs));
    kernelServices.basicConsole.Print(", DS = ");
    kernelServices.basicConsole.Println(to_hstring(ds));

    kernelServices.acpi.Initialize(&kernelServices.basicConsole, pBootInfo->rsdp);

    InitializeIDT(&kernelServices, pBootInfo);

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

    /*
     * Disable Timer Test
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
    */

    /*
     * Test I/O APIC
    */
    uint8_t gsi = 1; // IRQ1 for keyboard
    uint32_t gsib = kernelServices.ioapic.globalInterruptBase();

    if (gsi < gsib || gsi >= gsib + kernelServices.ioapic.redirectionEntries()) {
        kernelServices.basicConsole.Println("GSI out of range for this IOAPIC.");
    } else {
        uint8_t pin = gsi - gsib;

        RedirectionEntry entry = {};
        entry.vector = 0x21;         // IDT vector to call
        entry.delvMode = 0;          // fixed
        entry.destMode = 0;          // physical
        entry.mask = 0;              // ENABLED
        entry.triggerMode = 0;       // edge
        entry.pinPolarity = 0;       // high
        entry.destination = 0;       // LAPIC ID (usually 0 for BSP)

        kernelServices.ioapic.writeRedirEntry(pin, &entry);
        kernelServices.idt.SetDescriptor(0x21, (void*)irq_stub, 0x8E); // interrupt gate
    }

    kernelServices.basicConsole.Print("MADT Address: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.acpi.GetMADT()));
    kernelServices.basicConsole.Print("MADT Processor Local APIC: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.acpi.GetProcessorLocalAPIC()));
    kernelServices.basicConsole.Print("MADT I/O APIC: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernelServices.acpi.GetIOAPIC()));


    while (true) {
        
    }
    return 0;
}