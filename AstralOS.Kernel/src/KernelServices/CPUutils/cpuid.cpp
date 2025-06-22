#include "cpuid.h"

void cpuid(uint32_t code, uint32_t* eax, uint32_t* edx) {
    uint32_t ebx, ecx;
    asm volatile("cpuid"
                 : "=a"(*eax), "=d"(*edx), "=b"(ebx), "=c"(ecx)
                 : "a"(code));
}