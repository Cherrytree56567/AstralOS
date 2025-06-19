#pragma once
#include <cstdint>
#include "../Paging/PageTableManager/PageTableManager.h"
#include "../BasicConsole/BasicConsole.h"

/*
 * Found from https://wiki.osdev.org/Global_Descriptor_Table
*/

/*
 * Segment Descriptor
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
 * However, it is still good practice to use base and limit, in case we need to port to 32-bit.
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

    void set(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
        descriptor = 0;

        descriptor |= (limit & 0xFFFF);
        descriptor |= ((limit >> 16) & 0xF) << 48;

        descriptor |= (base & 0xFFFF) << 16;
        descriptor |= ((base >> 16) & 0xFF) << 32;
        descriptor |= ((base >> 24) & 0xFF) << 56;

        descriptor |= ((uint64_t)access) << 40;

        descriptor |= ((uint64_t)(flags & 0xF)) << 52;
    }
};

/*
 * System Segment Descriptor
*/
struct SystemAccessByte {
    uint8_t value;

    void setType(uint8_t type);
    void setS(bool val);
    void setDPL(uint8_t val);
    void setP(bool val);

    uint8_t getType() const {
        return value & 0xF;
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

/*
 * Types for the System Access Byte in 32-Bit Mode (Protected Mode)
 * Probably wont be used in this kernel, as it is 64-bit.
*/
enum class ProtectedModeType {
    TSS16A = 0x1,   // 16-bit TSS (Available)
    LDT = 0x2,      // LDT
    TSS16B = 0x3,   // 16-bit TSS (Busy)
    TSS32A = 0x9,   // 32-bit TSS (Available)
    TSS32B = 0xB    // 32-bit TSS (Busy)
};

enum class LongModeType {
    LDT = 0x2,      // LDT
    TSS64A = 0x9,   // 64-bit TSS (Available)
    TSS64B = 0xB    // 64-bit TSS (Busy)
};

struct SystemSegmentDescriptor {
    uint64_t descriptor;

    void set(uint32_t base, uint32_t limit, SystemAccessByte access, uint8_t flags) {
        descriptor = 0;

        descriptor |= (limit & 0xFFFF);
        descriptor |= ((limit >> 16) & 0xF) << 48;

        descriptor |= (base & 0xFFFF) << 16;
        descriptor |= ((base >> 16) & 0xFF) << 32;
        descriptor |= ((base >> 24) & 0xFF) << 56;

        descriptor |= ((uint64_t)access.value) << 40;

        descriptor |= ((uint64_t)(flags & 0xF)) << 52;
    }

    void set(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
        descriptor = 0;

        descriptor |= (limit & 0xFFFF);
        descriptor |= ((limit >> 16) & 0xF) << 48;

        descriptor |= (base & 0xFFFF) << 16;
        descriptor |= ((base >> 16) & 0xFF) << 32;
        descriptor |= ((base >> 24) & 0xFF) << 56;

        descriptor |= ((uint64_t)access) << 40;

        descriptor |= ((uint64_t)(flags & 0xF)) << 52;
    }
};

/*
 * Long mode Segment Descriptor
 * Takes up 2 entries.
 * 
 *  127 - 96    |   Reserved    |
 *  95  - 64    |     Base      |
 *  63  - 56    |     Base      |
 *  55  - 52    |    Flags      |
 *  51  - 48    |    Limit      |
 *  47  - 40    |  Access Byte  |
 *  39  - 32    |     Base      |
 *  31  - 16    |     Base      |
 *  15  -  0    |    Limit      |
*/
struct LongModeSegmentDescriptor {
    uint64_t low_descriptor;
    uint64_t high_descriptor;

    void set(uint64_t base, uint32_t limit, AccessByte access, uint8_t flags) {
        low_descriptor = 0;
        high_descriptor = 0;

        low_descriptor |= (limit & 0xFFFF);
        low_descriptor |= (base & 0xFFFF) << 16;
        low_descriptor |= ((base >> 16) & 0xFF) << 32;
        low_descriptor |= ((uint64_t)access.value) << 40;
        low_descriptor |= ((limit >> 16) & 0xF) << 48;
        low_descriptor |= ((flags & 0xF)) << 52;
        low_descriptor |= ((base >> 24) & 0xFF) << 56;

        high_descriptor |= (base >> 32) & 0xFFFFFFFF;
    }

    void set(uint64_t base, uint32_t limit, uint8_t access, uint8_t flags) {
        low_descriptor = 0;
        high_descriptor = 0;

        low_descriptor |= (limit & 0xFFFF);
        low_descriptor |= (base & 0xFFFF) << 16;
        low_descriptor |= ((base >> 16) & 0xFF) << 32;
        low_descriptor |= ((uint64_t)access) << 40;
        low_descriptor |= ((limit >> 16) & 0xF) << 48;
        low_descriptor |= ((flags & 0xF)) << 52;
        low_descriptor |= ((base >> 24) & 0xFF) << 56;

        high_descriptor |= (base >> 32) & 0xFFFFFFFF;
    }
};

struct GDTDescriptor {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));

struct GDTEntries {
    SegmentDescriptor nullSegment;      // 0
    SegmentDescriptor kernelCode;       // 1
    SegmentDescriptor kernelData;       // 2
    SegmentDescriptor userCode;         // 3
    SegmentDescriptor userData;         // 4
    //LongModeSegmentDescriptor tssDesc;  // 5 + 6 TODO: LATER
} __attribute__((packed));

class GDT {
public:
    GDT(PageTableManager* ptm, BasicConsole* bc) : pageTableManager(ptm), basicConsole(bc) {}

    void Create64BitGDT();

    void load_gdt(void* gdtr);
    void reload_segments();

private:
    PageTableManager* pageTableManager;
    BasicConsole* basicConsole;
    GDTEntries currentGDT;
};