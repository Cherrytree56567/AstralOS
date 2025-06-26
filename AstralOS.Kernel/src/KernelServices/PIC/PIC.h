#pragma once
#include <cstdint>
#include "../CPUutils/cpuid.h"

#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

/*
 * The info for this code is from OSDev
 * https://wiki.osdev.org/8259_PIC
*/
enum PICRegs {
    MPIC_Command = 0x0020,  // Master PIC - Command
    MPIC_Data = 0x0021,     // Master PIC - Data
    SPIC_Command = 0x00A0,  // Slave PIC - Command
    SPIC_Data = 0x00A1      // Slave PIC - Data
};

class PIC {
public:
    PIC() {}

    void RemapPIC(int offset1, int offset2);
    void SendEOI(uint8_t irq);
    void Disable();

    void SetMaskIRQ(uint8_t IRQline);
    void ClearMaskIRQ(uint8_t IRQline);
    
    uint16_t GetIRR();
    uint16_t GetISR();
private:
    uint16_t _GetIRQReg(int ocw3);
};