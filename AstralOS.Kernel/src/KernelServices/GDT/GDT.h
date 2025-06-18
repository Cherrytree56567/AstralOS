#pragma once
#include <cstdint>

/*
 * Found from https://wiki.osdev.org/Global_Descriptor_Table
*/

struct AccessByte {
    uint8_t value;

    void setAccessedBit(bool val);
    void setRW(bool val);
    void setDC(bool val);
    void setE(bool val);
    void setS(bool val);
    void setDPL(uint8_t val);
    void setP(bool val);

    bool getAccessedBit() const {
        return value & (1 << 0);
    }
    bool getRW() const {
        return value & (1 << 1);
    }
    bool getDC() const {
        return value & (1 << 2);
    }
    bool getE() const {
        return value & (1 << 3);
    }
    bool getS() const {
        return value & (1 << 4);
    }
    uint8_t getDPL() const {
        return (value >> 5) & 0x3;
    }
    bool getP() const {
        return value & (1 << 7);
    }
};

struct Flags {
    unsigned int value : 4;

    /*
     * Sets the Granularity flag.
     * If clear, the Limit is in 1 Byte blocks. 
     * If set, the Limit is in 4 KiB blocks (page granularity).
    */
    void setG(bool val) {
        value &= ~(1 << 3);
        value |= (val << 3);
    }

    /*
     * Sets the Size flag. 
     * If clear, the descriptor defines a 16-bit protected mode segment. 
     * If set, it defines a 32-bit protected mode segment.
    */
    void setDB(bool val) {
        value &= ~(1 << 2);
        value |= (val << 2);
    }

    /*
     * Sets the Long-mode code flag. 
     * If set, the descriptor defines a 64-bit code segment. When set, DB should always be clear.
    */
    void setL(bool val) {
        value &= ~(1 << 1);
        value |= (val << 1);
    }
};

/*
 * Base - A 32-bit value containing the linear address where the segment begins.
 * Limit - A 20-bit value, tells the maximum addressable unit, either in 1 byte units, or in 4KiB pages. 
 *         Hence, if you choose page granularity and set the Limit value to 0xFFFFF the segment will span the full 4 GiB address space in 32-bit mode.
 * 
 * In this case, since the kernel is booted in 64-bit mode, Base and Limit are ignored.
*/
struct SegmentDescriptor {
    uint64_t descriptor;

    void set(uint32_t base, uint32_t limit, AccessByte access, uint8_t flags) {
        descriptor = 0;

        descriptor |= (limit & 0xFFFF);
        descriptor |= ((limit >> 16) & 0xF) << 48;

        descriptor |= (base & 0xFFFF) << 16;
        descriptor |= ((base >> 16) & 0xFF) << 32;
        descriptor |= ((base >> 24) & 0xFF) << 56;

        descriptor |= ((uint64_t)access.value) << 40;

        descriptor |= ((uint64_t)(flags & 0xF)) << 52;
    }
};