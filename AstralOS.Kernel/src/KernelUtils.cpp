#include "KernelUtils.h"

#define HIGHER_VIRT_ADDR 0xFFFFFFFF00000000

/*
 * Most of the paging code is based on
 * https://github.com/Absurdponcho/PonchoOS
 * 
 * Not any more. ¯\_(ツ)_/¯
*/
extern "C" void InitializePaging(KernelServices* kernelServices, BootInfo* pBootInfo) {
    /*
     * Simple Memory Mapping.
     * 
     * Make sure we lock the kernel and the framebuffer
    */
    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_end = (uint64_t)&_kernel_end;

    uint64_t kernelSize = kernel_end - kernel_start;
    uint64_t kernelPages = kernelSize / 4096 + 1;

    uint64_t mMapEntries = pBootInfo->mMapSize / pBootInfo->mMapDescSize;
	uint64_t memorySize = GetMemorySize(pBootInfo->mMap, mMapEntries, pBootInfo->mMapDescSize);

	kernelServices->pageFrameAllocator.ReadEFIMemoryMap(pBootInfo->mMap, pBootInfo->mMapSize, pBootInfo->mMapDescSize);
    kernelServices->pageFrameAllocator.LockPages(&_kernel_start, kernelPages);

    kernelServices->PML4 = (PageTable*)kernelServices->pageFrameAllocator.RequestPage();
	memsetC(kernelServices->PML4, 0, 0x1000);
    kernelServices->pageTableManager.Initialize(kernelServices->PML4, &kernelServices->pageFrameAllocator, &kernelServices->basicConsole);
    kernelServices->pageTableManager.MapMemory((void*)kernelServices->PML4, (void*)kernelServices->PML4);

    for (uint64_t t = 0; t < memorySize; t += 0x1000){
        kernelServices->pageTableManager.MapMemory((void*)t, (void*)t);
    }

    uint64_t mMapPhys = (uint64_t)pBootInfo->mMap;
    uint64_t mMapSize = pBootInfo->mMapSize;
    uint64_t mMapPages = (mMapSize + 0xFFF) / 0x1000;

    for (uint64_t i = 0; i < mMapPages; i++) {
        kernelServices->pageTableManager.MapMemory(
            (void*)((uint64_t)pBootInfo->mMap + i * 0x1000),
            (void*)((uint64_t)pBootInfo->mMap + i * 0x1000)
        );

        kernelServices->pageTableManager.MapMemory(
            (void*)(HIGHER_VIRT_ADDR + (uint64_t)pBootInfo->mMap + i * 0x1000),
            (void*)((uint64_t)pBootInfo->mMap + i * 0x1000)
        );
    }

    uint64_t fbBase = (uint64_t)pBootInfo->pFramebuffer.BaseAddress;
    uint64_t fbSize = (uint64_t)pBootInfo->pFramebuffer.BufferSize + 0x1000;

    kernelServices->pageFrameAllocator.LockPages((void*)fbBase, fbSize / 4096 + 1);

    for (uint64_t t = fbBase; t < fbBase + fbSize; t += 4096) {
        kernelServices->pageTableManager.MapMemory((void*)(HIGHER_VIRT_ADDR + t), (void*)t);
        kernelServices->pageTableManager.MapMemory((void*)(t), (void*)t);
    }

    kernelServices->pageTableManager.MapMemory((void*)((uint64_t)pBootInfo->initrdBase + HIGHER_VIRT_ADDR), pBootInfo->initrdBase);
    kernelServices->pageTableManager.MapMemory((void*)((uint64_t)pBootInfo->rsdp + HIGHER_VIRT_ADDR), (void*)pBootInfo->rsdp);

    /*
     * We must translate our base addr and our ACPI Address.
     * Here, we are using a custom pBootInfo, because we may
     * not have access to it once we unmap the memory.
    */
    kernelServices->pBootInfo.rsdp = (void*)((uint64_t)HIGHER_VIRT_ADDR + (uint64_t)kernelServices->pBootInfo.rsdp);
    kernelServices->pBootInfo.pFramebuffer.BaseAddress = (void*)((uint64_t)HIGHER_VIRT_ADDR + (uint64_t)kernelServices->pBootInfo.pFramebuffer.BaseAddress);
    kernelServices->pBootInfo.initrdBase = (void*)((uint64_t)HIGHER_VIRT_ADDR + (uint64_t)kernelServices->pBootInfo.initrdBase);
    kernelServices->pBootInfo.mMap = (EFI_MEMORY_DESCRIPTOR*)(HIGHER_VIRT_ADDR + (uint64_t)pBootInfo->mMap);

    /*
     * Gotcha: 
     * Dont do `(HIGHER_VIRT_ADDR + kernelServices->pBootInfo.rsdp)`
     * because we already added the HIGHER_VIRT_ADDR before.
     */
    
    //

    uint64_t initrdPages = (pBootInfo->initrdSize + 0xFFF) / 0x1000;
    uint64_t initrdPhys = (uint64_t)pBootInfo->initrdBase;
    for (uint64_t i = 0; i < initrdPages; ++i) {
        void* physPage = (void*)(initrdPhys + i * 0x1000);
        kernelServices->pageTableManager.MapMemory(physPage, physPage);
        kernelServices->pageTableManager.MapMemory(
            (void*)(HIGHER_VIRT_ADDR + initrdPhys + i * 0x1000),
            physPage
        );
    }

    /*
     * Map and Set the APIC
    */
    kernelServices->pageTableManager.MapMemory((void*)0xFFFFFFFFFEE00000, (void*)0xFEE00000, false);
    kernelServices->apic.SetAPICBase(0xFFFFFFFFFEE00000);

    /*
     * Map the bitmap at the end,
     * so that we don't forget to 
     * map the new ones.
    */
    uint64_t bitmapPages = (kernelServices->pageFrameAllocator.GetBitmap().size + 4095) / 4096;
    for (uint64_t i = 0; i < bitmapPages; i++) {
        kernelServices->pageTableManager.MapMemory(
            (void*)((uint64_t)kernelServices->pageFrameAllocator.GetBitmap().buffer + i * 4096),
            (void*)((uint64_t)kernelServices->pageFrameAllocator.GetBitmap().buffer + i * 4096)
        );
    }

    uintptr_t kernelPhysBase = 0x1000;
    uintptr_t kernelVirtBase = HIGHER_VIRT_ADDR;
    uint64_t kPages = (kernelSize + 4095) / 4096;

    /*
     * Higher Half Mapping
    */
    for (uint64_t i = 0; i < kPages; i++) {
        kernelServices->pageTableManager.MapMemory(
            (void*)(kernelVirtBase + i * 4096),
            (void*)(kernelPhysBase + i * 4096)
        );
        kernelServices->pageTableManager.MapMemory(
            (void*)(kernelPhysBase + i * 4096),
            (void*)(kernelPhysBase + i * 4096)
        );
    }

    __asm__ volatile("mov %0, %%cr3" : : "r" (kernelServices->PML4));
}

IDTR64 idtr;
extern "C" void InitializeIDT(KernelServices* kernelServices, BootInfo* pBootInfo) {
    kernelServices->idt.CreateIDT();
    kernelServices->basicConsole.Println("Interrupts Initialized.");
    if (kernelServices->apic.CheckAPIC()) {
        kernelServices->basicConsole.Println("APIC is Supported.");

        /*
         * Enable APIC
         */
        kernelServices->apic.EnableAPIC();

        /*
        * Map I/O APIC
        */
        kernelServices->pageTableManager.MapMemory((void*)(HIGHER_VIRT_ADDR + kernelServices->acpi.GetIOAPIC()->IOAPIC_Addr), (void*)kernelServices->acpi.GetIOAPIC()->IOAPIC_Addr, false);
        /*
         * Initialize I/O APIC
         */
        kernelServices->ioapic.Initialize(&kernelServices->basicConsole, (HIGHER_VIRT_ADDR + kernelServices->acpi.GetIOAPIC()->IOAPIC_Addr), kernelServices->acpi.GetIOAPIC()->IOAPIC_ID, kernelServices->acpi.GetIOAPIC()->GSI_Base);
        kernelServices->basicConsole.Println("APIC is Enabled.");
    }

    __asm__ volatile ("sti");
}