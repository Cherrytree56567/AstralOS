#include "GDT.h"

/*
 * Segment Descriptor
*/

/*
 * Sets the Accessed Bit.
 * The CPU will set it when the segment is accessed unless set to 1 in advance.
*/
void AccessByte::setAccessedBit(bool val) {
    if (val) {
        value |= 1 << 0;
    } else {
        value &= ~(1 << 0);
    }
}

/*
 * Sets the Readable bit/Writable bit.
 * For code segments, If clear, read access for this segment is not allowed. If set, read access is allowed. Write access is never allowed for code segments.
 * For data segments, If clear, write access for this segment is not allowed. If set, write access is allowed. Read access is always allowed for data segments.
*/
void AccessByte::setRW(bool val) {
    if (val) {
        value |= (1 << 1);
    } else {
        value &= ~(1 << 1);
    }
}

/*
 * Sets the Direction/Conforming bit.
 * For Data Selectors, If clear, the segment grows up. If set, the segment grows down.
*/
void AccessByte::setDC(bool val) {
    if (val) {
        value |= (1 << 2);
    } else {
        value &= ~(1 << 2);
    }
}

/*
 * Sets the Executable Bit.
 * If clear, the descriptor defines a data segment.
 * If set, it defines a code segment which can be executed from.
*/
void AccessByte::setE(bool val) {
    if (val) {
        value |= (1 << 3);
    } else {
        value &= ~(1 << 3);
    }
}

/*
 * Sets the Descriptor type bit
 * If clear, the descriptor defines a system segment. 
 * If set, it defines a code or data segment.
*/
void AccessByte::setS(bool val) {
    if (val) {
        value |= (1 << 4);
    } else {
        value &= ~(1 << 4);
    }
}

/*
 * Sets the Descriptor privilege level field.
 * EG: setDPL(2); MAX IS 3
 * Contains the CPU Privilege level of the segment. 0 = highest privilege (kernel), 3 = lowest privilege (user applications).
*/
void AccessByte::setDPL(uint8_t val) {
    value &= ~(3 << 5);
    value |= (val & 0x3) << 5;
}

/*
 * Sets the Present bit
 * Allows an entry to refer to a valid segment. Must be set for any valid segment.
*/
void AccessByte::setP(bool val) {
    if (val) {
        value |= (1 << 7);
    } else {
        value &= ~(1 << 7);
    }
}

/*
 * System Segment Descriptor
*/

void SystemAccessByte::setType(uint8_t type) {
    value &= ~0x0F;
    value |= (type & 0x0F);
}

/*
 * Sets the Descriptor type bit
 * If clear, the descriptor defines a system segment. 
 * If set, it defines a code or data segment.
*/
void SystemAccessByte::setS(bool val) {
    if (val) {
        value |= (1 << 4);
    } else {
        value &= ~(1 << 4);
    }
}

/*
 * Sets the Descriptor privilege level field.
 * EG: setDPL(2); MAX IS 3
 * Contains the CPU Privilege level of the segment. 0 = highest privilege (kernel), 3 = lowest privilege (user applications).
*/
void SystemAccessByte::setDPL(uint8_t val) {
    value &= ~(3 << 5);
    value |= (val & 0x3) << 5;
}

/*
 * Sets the Present bit
 * Allows an entry to refer to a valid segment. Must be set for any valid segment.
*/
void SystemAccessByte::setP(bool val) {
    if (val) {
        value |= (1 << 7);
    } else {
        value &= ~(1 << 7);
    }
}

void GDT::Initialize(BasicConsole* bc) {
    basicConsole = bc;
}

void GDT::Create64BitGDT() {
    /*
     * These values are set according to the OSDev Wiki.
     * https://wiki.osdev.org/GDT_Tutorial
    */

    /*
     * Null Descriptor
     * Offset = 0x0000
    */
    currentGDT.nullSegment.set(0, 0x00000000, 0x00, 0x0);

    /*
     * Kernel Mode Code Segment
     * Offset = 0x0008
    */
    currentGDT.kernelCode.set(0, 0xFFFFF, 0x9A, 0xA);

    /*
     * Kernel Mode Data Segment
     * Offset = 0x0010
    */
    currentGDT.kernelData.set(0, 0xFFFFF, 0x92, 0xC);

    /*
     * User Mode Code Segment
     * Offset = 0x0018
    */
    currentGDT.userCode.set(0, 0xFFFFF, 0xFA, 0xA);

    /*
     * User Mode Data Segment
     * Offset = 0x0020
    */
    currentGDT.userData.set(0, 0xFFFFF, 0xF2, 0xC);

    /*
     * Task State Segment
     * Offset = 0x0028
    */
    //currentGDT.tssDesc.set(, , 0x89, 0x0);
    // TODO: Add the TSS before Ring 3

    GDTDescriptor gdtr = {
        .size = sizeof(GDTEntries) - 1,
        .offset = (uint64_t)&currentGDT
    };

    load_gdt(&gdtr);
    reload_segments();
}

void GDT::load_gdt(void* gdtr) {
    asm volatile (
        "lgdt (%0)"
        :
        : "r" (gdtr)
        : "memory"
    );
}

void GDT::reload_segments() {
    asm volatile (
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"

        "pushq $0x08\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n"
        "1:\n\t"
        :
        :
        : "rax", "memory"
    );
}