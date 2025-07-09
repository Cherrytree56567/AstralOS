#pragma once
#include "KernelServices/KernelServices.h"
#include "cstr/cstr.h"

extern "C" {
    extern uint8_t _kernel_start;
    extern uint8_t _kernel_end;
}

extern "C" int start(KernelServices& kernelServices, BootInfo* pBootInfo);