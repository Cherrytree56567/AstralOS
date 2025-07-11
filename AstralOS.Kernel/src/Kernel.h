#pragma once
#include "KernelServices/KernelServices.h"
#include "cstr/cstr.h"

extern "C" {
    extern uint8_t _kernel_start;
    extern uint8_t _kernel_end;
    extern uint8_t _stack_end;
    extern uint8_t _stack_start;
}

extern "C" int start(KernelServices& kernelServices, BootInfo* pBootInfo);