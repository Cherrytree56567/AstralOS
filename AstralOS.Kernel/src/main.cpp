#include "KernelUtils.h"

extern "C" int _start(BootInfo* pBootInfo) {
	KernelServices kernelServices(pBootInfo);

    InitializePaging(&kernelServices, pBootInfo);

    memset(pBootInfo->pFramebuffer->BaseAddress, 0, pBootInfo->pFramebuffer->BufferSize);
    kernelServices.basicConsole.Println("Framebuffer Cleared");

    kernelServices.pageTableManager.MapMemory((void*)0x600000000, (void*)0x80000);
	kernelServices.basicConsole.Println("Mapped 0x600000000 to 0x80000");

    uint64_t* test = (uint64_t*)0x600000000;
    *test = 26;

    kernelServices.basicConsole.Println(to_string(*test));

    while (true) {
        
    }
    return 0;
}