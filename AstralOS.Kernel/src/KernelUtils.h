#include "Kernel.h"

extern "C" void InitializePaging(KernelServices* kernelServices, BootInfo* pBootInfo);
extern "C" void InitializeIDT(KernelServices* KernelServices, BootInfo* pBootInfo);