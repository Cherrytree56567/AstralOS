#pragma once
#include <cstdint>
#include <cstddef>

/*
 * CPUID enums from OSDev
 * https://wiki.osdev.org/Talk:CPUID
*/
enum {
    CPUID_FEAT_ECX_SSE3         = 1 << 0,  // streaming SIMD extensions 3 (SSE3)
    CPUID_FEAT_ECX_MONITOR      = 1 << 3,  // monitor/mwait
    CPUID_FEAT_ECX_DS_CPL       = 1 << 4,  // CPL qualified debug store
    CPUID_FEAT_ECX_VMX          = 1 << 5,  // virtual machine extensions
    CPUID_FEAT_ECX_SMX          = 1 << 6,  // safer mode extensions
    CPUID_FEAT_ECX_EST          = 1 << 7,  // enhanced Intel SpeedStep(R) technology
    CPUID_FEAT_ECX_TM2          = 1 << 8,  // thermal monitor 2
    CPUID_FEAT_ECX_SSSE3        = 1 << 9,  // supplemental streaming SIMD extensions 3 (SSSSE3)
    CPUID_FEAT_ECX_CNXT_ID      = 1 << 10, // L1 context ID
    CPUID_FEAT_ECX_CMPXCHG16B   = 1 << 13, // cmpxchg16b available (obviously)
    CPUID_FEAT_ECX_xTPR_UPDATE  = 1 << 14, // xTPR update control
    CPUID_FEAT_ECX_PDCM         = 1 << 15, // performance and debug capability
    CPUID_FEAT_ECX_DCA          = 1 << 18, // memory-mapped device prefetching
    CPUID_FEAT_ECX_SSE4_1       = 1 << 19, // SSE4.1
    CPUID_FEAT_ECX_SSE4_2       = 1 << 20, // SSE4.2
    CPUID_FEAT_ECX_x2APIC       = 1 << 21, // x2APIC available
    CPUID_FEAT_ECX_MOVBE        = 1 << 22, // movbe available
    CPUID_FEAT_ECX_POPCNT       = 1 << 23, // popcnt available (sounds rude)
    CPUID_FEAT_ECX_XSAVE        = 1 << 26, // xsave/xrstor/xsetbv/xgetbv supported
    CPUID_FEAT_ECX_OSXSAVE      = 1 << 27, // xsetbv/xgetbv has been enabled

    CPUID_FEAT_EDX_x87          = 1 << 0,  // x86 FPU on chip
    CPUID_FEAT_EDX_VME          = 1 << 1,  // virtual-8086 mode enhancement
    CPUID_FEAT_EDX_DE           = 1 << 2,  // debugging extensions
    CPUID_FEAT_EDX_PSE          = 1 << 3,  // page size extensions
    CPUID_FEAT_EDX_TSC          = 1 << 4,  // timestamp counter
    CPUID_FEAT_EDX_MSR          = 1 << 5,  // rdmsr/wrmsr
    CPUID_FEAT_EDX_PAE          = 1 << 6,  // page address extensions
    CPUID_FEAT_EDX_MCE          = 1 << 7,  // machine check exception
    CPUID_FEAT_EDX_CX8          = 1 << 8,  // cmpxchg8b supported
    CPUID_FEAT_EDX_APIC         = 1 << 9,  // APIC on a chip
    CPUID_FEAT_EDX_SEP          = 1 << 11, // sysenter/sysexit
    CPUID_FEAT_EDX_MTRR         = 1 << 12, // memory type range registers
    CPUID_FEAT_EDX_PGE          = 1 << 13, // PTE global bit (PTE_GLOBAL)
    CPUID_FEAT_EDX_MCA          = 1 << 14, // machine check architecture
    CPUID_FEAT_EDX_CMOV         = 1 << 15, // conditional move/compare instructions
    CPUID_FEAT_EDX_PAT          = 1 << 16, // page attribute table
    CPUID_FEAT_EDX_PSE36        = 1 << 17, // page size extension
    CPUID_FEAT_EDX_PSN          = 1 << 18, // processor serial number
    CPUID_FEAT_EDX_CLFSH        = 1 << 19, // cflush instruction
    CPUID_FEAT_EDX_DS           = 1 << 21, // debug store
    CPUID_FEAT_EDX_ACPI         = 1 << 22, // thermal monitor and clock control
    CPUID_FEAT_EDX_MMX          = 1 << 23, // MMX technology
    CPUID_FEAT_EDX_FXSR         = 1 << 24, // fxsave/fxrstor
    CPUID_FEAT_EDX_SSE          = 1 << 25, // SSE extensions
    CPUID_FEAT_EDX_SSE2         = 1 << 26, // SSE2 extensions, obviously
    CPUID_FEAT_EDX_SS           = 1 << 27, // self-snoop
    CPUID_FEAT_EDX_HTT          = 1 << 28, // multi-threading (hyper-threading, I think - the afterburner of Intel CPUs)
    CPUID_FEAT_EDX_TM           = 1 << 29, // thermal monitor
    CPUID_FEAT_EDX_PBE          = 1 << 31, // Pend. Brk. EN. (wtf?)
};

void cpuid(uint32_t code, uint32_t* eax, uint32_t* edx);
void outb(unsigned short port, unsigned char val);
uint8_t inb(uint16_t port);
void outl(uint16_t port, uint32_t val);
uint32_t inl(uint16_t port);