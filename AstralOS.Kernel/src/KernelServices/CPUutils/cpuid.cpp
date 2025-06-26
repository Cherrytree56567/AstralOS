#include "cpuid.h"

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