#include "Timer.h"
#include "../KernelServices.h"

void APICTimer::Initialize(uint32_t vector, bool periodic, uint32_t initial_count, uint8_t dc) {
    divide_config = dc;
    ks->apic.WriteAPIC(APICRegs::DCT, dc);

    uint32_t lvt_timer = vector & 0xFF;
    if (periodic) {
        lvt_timer |= (1 << 17);
    } else {
        lvt_timer &= ~(1 << 17);
    }

    ks->apic.WriteAPIC(APICRegs::lvtTimer, lvt_timer);

    ks->apic.WriteAPIC(APICRegs::ICT, initial_count);

    ks->apic.WriteAPIC(APICRegs::CCT, 0);
}

void APICTimer::sleep(uint64_t ms) {
    if (ticks_per_ms == 0) {
        ks->basicConsole.Println("APICTimer not calibrated!");
        return;
    }

    uint32_t start = ks->apic.ReadAPIC(APICRegs::CCT);
    uint32_t ticks_needed = ms * ticks_per_ms;

    while ((start - ks->apic.ReadAPIC(APICRegs::CCT)) < ticks_needed) {
        
    }
}

void APICTimer::Calibrate() {
    const uint32_t pit_delay_ms = 50;

    uint32_t initial_count = 0xFFFFFFFF;
    Initialize(0x20, false, initial_count, divide_config);

    ks->pit.pit_wait(pit_delay_ms);

    while (!ks->pit.pit_expired()) {
        
    }

    uint32_t current_count = ks->apic.ReadAPIC(APICRegs::CCT);
    uint32_t ticks_elapsed = initial_count - current_count;

    ticks_per_ms = ticks_elapsed / pit_delay_ms;
}