#include "Kernel.h"

#define KERNEL_VIRT_ADDR 0xFFFFFFFF80000000ULL
#define HIGHER_VIRT_ADDR 0xFFFFFFFF00000000

extern "C" void InitializePaging(KernelServices* kernelServices, BootInfo* pBootInfo);
extern "C" void InitializeIDT(KernelServices* KernelServices, BootInfo* pBootInfo);