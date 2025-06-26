#include "PIC.h"

/*
 * The code is from OSDev
 * https://wiki.osdev.org/8259_PIC
*/

void io_wait() {
    outb(0x80, 0);
}

/*
 * Starts the initialization sequence (in cascade mode)
 * ICW2: Master PIC vector offset
 * ICW2: Slave PIC vector offset
 * ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
 * ICW3: tell Slave PIC its cascade identity (0000 0010)
 * ICW4: have the PICs use 8086 mode (and not 8080 mode)
 * 
 * Unmask both PICs.
*/
void PIC::RemapPIC(int offset1, int offset2) {
	outb(PICRegs::MPIC_Command, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PICRegs::SPIC_Command, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PICRegs::MPIC_Data, offset1);
	io_wait();
	outb(PICRegs::SPIC_Data, offset2);
	io_wait();
	outb(PICRegs::MPIC_Data, 4);
	io_wait();
	outb(PICRegs::SPIC_Data, 2);
	io_wait();

	outb(PICRegs::MPIC_Data, ICW4_8086);
	io_wait();
	outb(PICRegs::SPIC_Data, ICW4_8086);
	io_wait();

	outb(PICRegs::MPIC_Data, 0);
	outb(PICRegs::SPIC_Data, 0);
}

void PIC::SendEOI(uint8_t irq) {
	if (irq >= 8) {
		outb(PICRegs::SPIC_Command, 0x20);
    }

	outb(PICRegs::MPIC_Command, 0x20);
}

void PIC::Disable(void) {
    outb(PICRegs::MPIC_Data, 0xff);
    outb(PICRegs::SPIC_Data, 0xff);
}

void PIC::SetMaskIRQ(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if (IRQline < 8) {
        port = PICRegs::MPIC_Data;
    } else {
        port = PICRegs::SPIC_Data;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);        
}

void PIC::ClearMaskIRQ(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if (IRQline < 8) {
        port = PICRegs::MPIC_Data;
    } else {
        port = PICRegs::SPIC_Data;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);        
}

/*
 * OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
 * represents IRQs 8-15. PIC1 is IRQs 0-7, with 2 being the chain
*/
uint16_t PIC::_GetIRQReg(int ocw3) {
    outb(PICRegs::MPIC_Command, ocw3);
    outb(PICRegs::SPIC_Command, ocw3);
    return (inb(PICRegs::SPIC_Command) << 8) | inb(PICRegs::MPIC_Command);
}

/* 
 * Returns the combined value of the cascaded PICs irq request register 
 */
uint16_t PIC::GetIRR(void) {
    return _GetIRQReg(0x0a);
}

/* 
 * Returns the combined value of the cascaded PICs in-service register 
 */
uint16_t PIC::GetISR(void) {
    return _GetIRQReg(0x0b);
}