#pragma once
#include <cstdint>
#include <cstddef>
#include "../APIC/APIC.h"
#include "../PIT/PIT.h"

class APICTimer {
public:
    void Initialize(uint32_t vector, bool periodic, uint32_t initial_count, uint8_t divide_config);

    void sleep(uint64_t ms);
    void Calibrate();
public:
    volatile uint64_t g_ticks = 0;

private:
    uint32_t ticks_per_ms = 0;
    uint8_t divide_config = 0x03;
};