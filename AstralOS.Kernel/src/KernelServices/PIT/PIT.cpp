#include "PIT.h"

unsigned PIT::read_pit_count(void) {
	unsigned count = 0;
	
	asm volatile("cli");
	
	outb(0x43,0b0000000);
	
	count = inb(0x40);
	count |= inb(0x40)<<8;

    asm volatile("sti");
	
	return count;
}

void PIT::set_pit_count(unsigned count) {
	asm volatile("cli");
	
	outb(0x40,count&0xFF);
	outb(0x40,(count&0xFF00)>>8);

    asm volatile("sti");
	return;
}

void PIT::pit_wait(uint32_t ms) {   
    uint32_t count = (1193182 * ms) / 1000;

    outb(PIT_COMMAND, 0x30);
    outb(PIT_CHANNEL0, count & 0xFF);
    outb(PIT_CHANNEL0, (count >> 8) & 0xFF);
}

bool PIT::pit_expired() {
    outb(0x43, 0x00);

    uint8_t low  = inb(0x40);
    uint8_t high = inb(0x40);
    uint16_t count = (high << 8) | low;

    return (count == 0);
}