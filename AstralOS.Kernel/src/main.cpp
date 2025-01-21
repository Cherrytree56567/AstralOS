#include "Kernel.h"

extern "C" int _start(BootInfo* pBootInfo) {
	KernelServices kernelServices(pBootInfo);

    // Call Print function with a test string
    kernelServices.basicConsole.Print("Hello Kernel\n");
	kernelServices.basicConsole.Println(to_string((uint64_t)1234976));
	kernelServices.basicConsole.Println(to_string((int64_t)-1234976));
	kernelServices.basicConsole.Println(to_hstring((uint64_t)0xF0));
	kernelServices.basicConsole.Println(to_hstring((uint32_t)0xF0));
	kernelServices.basicConsole.Println(to_hstring((uint16_t)0xF0));
	kernelServices.basicConsole.Println(to_hstring((uint8_t)0xF0));
	kernelServices.basicConsole.Println(to_string((double)-72.11)); // TODO: FIX THIS, -72.109999999999999
	kernelServices.basicConsole.Println(to_string((double)-72.163));
	kernelServices.basicConsole.Println(to_string((double)-72.163876, 5));

    uint64_t kernel_start = 0x1000; // This will be defined via the linker

    extern uint64_t _end; // Declare _end as an external symbol

    // Define _end in the .bss section

    kernelServices.basicConsole.Print("Kernel Start: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t)kernel_start));
    __asm__(".global _end; _end:");
    kernelServices.basicConsole.Print("Kernel End: ");
    kernelServices.basicConsole.Println(to_hstring((uint64_t) & _end));

    uint64_t kernel_end = _end;

    uint64_t kernelSize = (uint64_t)&kernel_end - (uint64_t)&kernel_start;
    uint64_t kernelPages = (uint64_t)kernelSize / 4096 + 1;
    kernelServices.basicConsole.Print("Kernel End1:");

    uint64_t mMapEntries = pBootInfo->mMapSize / pBootInfo->mMapDescSize;
	uint64_t memorySize = GetMemorySize(pBootInfo->mMap, mMapEntries, pBootInfo->mMapDescSize);
	uint64_t bitmapSize = memorySize / 4096 / 8 + 1;

	kernelServices.pageFrameAllocator.ReadEFIMemoryMap(pBootInfo->mMap, pBootInfo->mMapSize, pBootInfo->mMapDescSize);
	kernelServices.basicConsole.Println("Memory Map Read");

    for (uint64_t t = 0; t < GetMemorySize(pBootInfo->mMap, mMapEntries, pBootInfo->mMapDescSize); t += 0x1000) {
        kernelServices.pageTableManager->MapMemory((void*)t, (void*)t);
		kernelServices.basicConsole.Println("1");
    }

    uint64_t fbBase = (uint64_t)pBootInfo->pFramebuffer->BaseAddress;
    uint64_t fbSize = (uint64_t)pBootInfo->pFramebuffer->BufferSize + 0x1000;
    for (uint64_t t = fbBase; t < fbBase + fbSize; t += 4096) {
        kernelServices.pageTableManager->MapMemory((void*)t, (void*)t);
		kernelServices.basicConsole.Println("2");
    }

    __asm__("mov %0, %%cr3" : : "r" (kernelServices.pageTableManager->PML4));
	kernelServices.basicConsole.Println("CR3 Set");

    kernelServices.pageTableManager->MapMemory((void*)0x600000000, (void*)0x8);
	kernelServices.basicConsole.Println("Mapped 0x600000000 to 0x8");

    uint64_t* test = (uint64_t*)0x600000000;
    *test = 26;

    kernelServices.basicConsole.Println(to_string(*test));

    return 0;
}