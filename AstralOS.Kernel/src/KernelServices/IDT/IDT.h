#pragma once
#include <cstdint>
#include "../GDT/GDT.h"
#include "../BasicConsole/BasicConsole.h"

#define IDT_MAX_DESCRIPTORS 	256

/*
 * Defines from Poncho OS
 * https://github.com/Absurdponcho/PonchoOS/blob/Episode-12-More-IDT/kernel/src/interrupts/IDT.h
*/
#define IDT_TA_InterruptGate    0b10001110
#define IDT_TA_CallGate         0b10001100
#define IDT_TA_TrapGate         0b10001111

/*
 * All structs are from OSDev
 * https://wiki.osdev.org/Interrupts_Tutorial
*/

/*
 * 32 Bit IDT Entry
 * Probably won't be used since this is a 64-Bit Kernel.
*/
struct IDT_ENTRY32 {
    uint16_t isr_low;      // The lower 16 bits of the ISR's address
	uint16_t kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t reserved;      // Set to zero
	uint8_t attributes;    // Type and attributes; see the IDT page
	uint16_t isr_high;     // The higher 16 bits of the ISR's address
} __attribute__((packed));

/*
 * 64 Bit IDT Entry
*/
struct IDT_ENTRY64 {
    uint16_t isr_low;      // The lower 16 bits of the ISR's address
	uint16_t kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	ist;           // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t attributes;    // Type and attributes; see the IDT page
	uint16_t isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t isr_high;     // The higher 32 bits of the ISR's address
	uint32_t reserved;     // Set to zero
} __attribute__((packed));

/*
 * 32 Bit IDT Reference
 * Probably not going to be used, as this is a 64 Bit Kernel.
*/
struct IDTR32 {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

/*
 * 64 Bit IDT Reference
*/
struct IDTR64 {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

extern "C" void exception_handler();

class IDT {
public:
	IDT() {}

	void Initialize(BasicConsole* bc);

	void SetDescriptor(uint8_t vector, void* isr, uint8_t flags);
	void CreateIDT();

private:
	BasicConsole* basicConsole;

	IDTR64 idtr;

	__attribute__((aligned(0x10))) 
	IDT_ENTRY64 idt[256];
};