#include "IDT.h"
#include "../KernelServices.h"

extern "C" void exception_handler() {
    ks->basicConsole.Print("EXCEPTION");
    while (true) __asm__ volatile ("cli; hlt");
}

void IDT::Initialize(BasicConsole* bc) {
    basicConsole = bc;
}

/*
 * 64 Bit Set Descriptor
 * Only for 64 Bit Kernels
 * PS: If you want to use my
 * code, go ahead, just credit
 * me and OSDev.org
*/
void IDT::SetDescriptor(uint8_t vector, void* isr, uint8_t flags) {
	IDT_ENTRY64* descriptor = &idt[vector];

	descriptor->isr_low = (uint64_t)isr & 0xFFFF;
	descriptor->kernel_cs = GDTOffsets::GDT_KERNEL_CODE;
	descriptor->ist = 0;
	descriptor->attributes = flags;
	descriptor->isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
	descriptor->isr_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
	descriptor->reserved = 0;
}

static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];

void IDT::CreateIDT() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(IDT_ENTRY64) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t vector = 0; vector < 32; vector++) {
        SetDescriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr));
    __asm__ volatile ("sti");
}