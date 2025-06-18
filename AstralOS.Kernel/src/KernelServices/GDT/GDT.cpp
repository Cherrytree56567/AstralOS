#include "GDT.h"

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