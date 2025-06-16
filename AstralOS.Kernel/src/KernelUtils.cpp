#include "KernelUtils.h"

/*
 * Most of the paging code is based on
 * https://github.com/Absurdponcho/PonchoOS
*/
extern "C" void InitializePaging(KernelServices* kernelServices, BootInfo* pBootInfo) {
    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_end = (uint64_t)&_kernel_end;

    uint64_t kernelSize = kernel_end - kernel_start;
    uint64_t kernelPages = kernelSize / 4096 + 1;

    uint64_t mMapEntries = pBootInfo->mMapSize / pBootInfo->mMapDescSize;
	uint64_t memorySize = GetMemorySize(pBootInfo->mMap, mMapEntries, pBootInfo->mMapDescSize);

	kernelServices->pageFrameAllocator.ReadEFIMemoryMap(pBootInfo->mMap, pBootInfo->mMapSize, pBootInfo->mMapDescSize);
    kernelServices->pageFrameAllocator.LockPages(&_kernel_start, kernelPages);

    kernelServices->PML4 = (PageTable*)kernelServices->pageFrameAllocator.RequestPage();
	memset(kernelServices->PML4, 0, 0x1000);
    kernelServices->pageTableManager.Initialize(kernelServices->PML4, &kernelServices->pageFrameAllocator, &kernelServices->basicConsole);

    for (uint64_t t = 0; t < memorySize; t += 0x1000){
        kernelServices->pageTableManager.MapMemory((void*)t, (void*)t);
    }

    uint64_t fbBase = (uint64_t)pBootInfo->pFramebuffer->BaseAddress;
    uint64_t fbSize = (uint64_t)pBootInfo->pFramebuffer->BufferSize + 0x1000;

    kernelServices->pageFrameAllocator.LockPages((void*)fbBase, fbSize / 4096 + 1);

    for (uint64_t t = fbBase; t < fbBase + fbSize; t += 4096) {
        kernelServices->pageTableManager.MapMemory((void*)t, (void*)t);
    }

    __asm__("mov %0, %%cr3" : : "r" (kernelServices->PML4));
}
