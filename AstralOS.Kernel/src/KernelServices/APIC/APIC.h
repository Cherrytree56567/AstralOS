#pragma once
#include <cstdint>
#include "../../Utils/cpu.h"
#include "../PIC/PIC.h"

/*
 * This code is from OSDev
 * https://wiki.osdev.org/APIC
*/
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

/*
 * This code is from ChatGPT
 * ik, chatgpt is bad, but the OSDev article was hard to understand.
*/
#define APIC_REG(base, offset) (*((volatile uint32_t *)((base) + (offset))))

/*
 * From OSDev (https://wiki.osdev.org/APIC)
 * 
 * TODO: Add RW Table to prevent unauthorised access to APIC Registers.
 * 
 * --EOI Register--
 * Write to the register with offset 0xB0 using the value 0 to signal an end of interrupt. 
 * A non-zero value may cause a general protection fault. 
 * 
 * --Spurious Interrupt Vector Register--
 * The offset is 0xF0. The low byte contains the number of the spurious interrupt. 
 * As noted above, you should probably set this to 0xFF. To enable the APIC, set bit 8 (or 0x100) of this register. 
 * If bit 12 is set then EOI messages will not be broadcast. All the other bits are currently reserved. 
*/
enum class APICRegs {
    Reserved0 = 0x000,  // Reserved
    Reserved1 = 0x010,  // Reserved
    lapicID = 0x020,    // LAPIC ID Register (Read/Write)
    lapicVer = 0x030,   // LAPIC Version Register (Read only)
    Reserved2 = 0x040,  // Reserved
    Reserved3 = 0x050,  // Reserved
    Reserved4 = 0x060,  // Reserved
    Reserved5 = 0x070,  // Reserved
    TPR = 0x080,        // Task Priority Register (Read/Write)
    APR = 0x090,        // Arbitration Priority Register (Read only)
    PPR = 0x0A0,        // Processor Priority Register (Read only)
    EOI = 0x0B0,        // EOI Register (Write only)
    RRD = 0x0C0,        // Remote Read Register (Read only)
    LD = 0x0D0,         // Logical Destination Register (Read/Write)
    DF = 0x0E0,         // Destination Format Register (Read/Write)
    SIV = 0x0F0,        // Spurious Interrupt Vector Register (Read/Write)
    ISR0 = 0x100,       // In-Service Register 0 (Read only)
    ISR1 = 0x110,       // In-Service Register 1 (Read only)
    ISR2 = 0x120,       // In-Service Register 2 (Read only)
    ISR3 = 0x130,       // In-Service Register 3 (Read only)
    ISR4 = 0x140,       // In-Service Register 4 (Read only)
    ISR5 = 0x150,       // In-Service Register 5 (Read only)
    ISR6 = 0x160,       // In-Service Register 6 (Read only)
    ISR7 = 0x170,       // In-Service Register 7 (Read only)
    TMR0 = 0x180,       // Trigger Mode Register 0 (Read only)
    TMR1 = 0x190,       // Trigger Mode Register 1 (Read only)
    TMR2 = 0x1A0,       // Trigger Mode Register 2 (Read only)
    TMR3 = 0x1B0,       // Trigger Mode Register 3 (Read only)
    TMR4 = 0x1C0,       // Trigger Mode Register 4 (Read only)
    TMR5 = 0x1D0,       // Trigger Mode Register 5 (Read only)
    TMR6 = 0x1E0,       // Trigger Mode Register 6 (Read only)
    TMR7 = 0x1F0,       // Trigger Mode Register 7 (Read only)
    IRR0 = 0x200,       // Interrupt Request Register 0 (Read only)
    IRR1 = 0x210,       // Interrupt Request Register 1 (Read only)
    IRR2 = 0x220,       // Interrupt Request Register 2 (Read only)
    IRR3 = 0x230,       // Interrupt Request Register 3 (Read only)
    IRR4 = 0x240,       // Interrupt Request Register 4 (Read only)
    IRR5 = 0x250,       // Interrupt Request Register 5 (Read only)
    IRR6 = 0x260,       // Interrupt Request Register 6 (Read only)
    IRR7 = 0x270,       // Interrupt Request Register 7 (Read only)
    ErrStat = 0x280,    // Error Status Register (Read only)
    Reserved6 = 0x290,  // Reserved
    Reserved7 = 0x2A0,  // Reserved
    Reserved8 = 0x2B0,  // Reserved
    Reserved9 = 0x2C0,  // Reserved
    Reserved10 = 0x2D0, // Reserved
    Reserved11 = 0x2E0, // Reserved
    CMCI = 0x2F0,       // LVT Corrected Machine Check Interrupt (CMCI) Register (Read/Write)
    ICR0 = 0x300,       // Interrupt Command Register (Read/Write)
    ICR1 = 0x310,       // Interrupt Command Register (Read/Write)
    lvtTimer = 0x320,   // LVT Timer Register (Read/Write)
    lvtThermal = 0x330, // LVT Thermal Sensor Register (Read/Write)
    lvtPerf = 0x340,    // LVT Performance Monitoring Counters Register (Read/Write)
    LINT0 = 0x350,      // LVT LINT0 Register (Read/Write)
    LINT1 = 0x360,      // LVT LINT1 Register (Read/Write)
    LVTErr = 0x370,     // LVT Error Register (Read/Write)
    ICT = 0x380,        // Initial Count Register (for Timer) (Read/Write)
    CCT = 0x390,        // Current Count Register (for Timer) (Read/Write)
    Reserved12 = 0x3A0, // Reserved
    Reserved13 = 0x3B0, // Reserved
    Reserved14 = 0x3C0, // Reserved
    Reserved15 = 0x3D0, // Reserved
    DCT = 0x3E0,        // Divide Configuration Register (for Timer) (Read/Write)
    Reserved16 = 0x3F0  // Reserved
};

struct LocalVectorTable {
    uint32_t value;

    uint8_t GetVector();
    uint8_t GetDeliveryMode();
    bool GetInterruptPending();
    bool GetPolarity();
    bool GetRemoteIRR();
    bool GetTriggerMode();
    bool GetMasked();
    void SetVector(uint8_t val);
    void SetDeliveryMode(uint8_t val);
    void SetInterruptPending(bool flag);
    void SetPolarity(bool flag);
    void SetRemoteIRR(bool flag);
    void SetTriggerMode(bool flag);
    void SetMasked(bool flag);
} __attribute__((packed));

struct InterruptCommandRegister {
    uint32_t value;

    uint8_t GetVector();
    uint8_t GetDeliveryMode();
    bool GetDestinationMode();
    bool GetDeliveryStatus();
    bool GetInitClear();
    bool GetInitSet();
    uint8_t GetDestinationType();
    void SetVector(uint8_t val);
    void SetDeliveryMode(uint8_t val);
    void SetDestinationMode(bool flag);
    void SetDeliveryStatus(bool flag);
    void SetInitClear(bool flag);
    void SetInitSet(bool flag);
    void SetDestinationType(uint8_t flag);
} __attribute__((packed));

/*
 * 32 Bit APIC is the same for 64 Bit APIC
*/
class APIC {
public:
    APIC() {}

    bool CheckAPIC();
    void SetAPICBase(uintptr_t apic);
    uintptr_t GetAPICBase();
    void EnableAPIC();

    bool IsInterruptPending();

    void ReadMSR(uint32_t msr, uint32_t* eax, uint32_t* edx);
    void WriteMSR(uint32_t msr, uint32_t eax, uint32_t edx);

    uint32_t ReadAPIC(APICRegs reg);
    void WriteAPIC(APICRegs reg, uint32_t val);
};