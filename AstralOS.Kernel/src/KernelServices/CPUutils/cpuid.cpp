#include "cpuid.h"

/*
 * All of this code is from ChadGPT.
*/
void cpuid(uint32_t code, uint32_t* eax, uint32_t* edx) {
    uint32_t ebx, ecx;
    asm volatile("cpuid"
                 : "=a"(*eax), "=d"(*edx), "=b"(ebx), "=c"(ecx)
                 : "a"(code));
}

void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0"
                      : "=a"(ret)
                      : "Nd"(port));
    return ret;
}

void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

uint32_t inl(uint16_t port) {
    uint32_t val;
    // x86 I/O‑port read: ALU ← [port]
    asm volatile (
        "inl %%dx, %%eax"
        : "=a"(val)        // output: EAX → val
        : "d"(port)        // input:  DX ← port
        : /* no clobbers */
    );
    return val;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }
    return (unsigned char)(*s1) - (unsigned char)(*s2);
}