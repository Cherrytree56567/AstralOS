#pragma once
#include <cstdint>
#include "../../Utils/cpu.h"

enum IOPorts {
    PIT_CHANNEL0 = 0x40,
    PIT_CHANNEL1 = 0x41,
    PIT_CHANNEL2 = 0x42,
    PIT_COMMAND  = 0x43
};

/*
 * From the OSDev Wiki:
 * https://wiki.osdev.org/Programmable_Interval_Timer#Channel_0
*/
class PIT {
public:
    unsigned read_pit_count(void);
    void set_pit_count(unsigned count);
    void pit_wait(uint32_t ms);
    bool pit_expired();

private:
    volatile uint32_t CountDown;
};