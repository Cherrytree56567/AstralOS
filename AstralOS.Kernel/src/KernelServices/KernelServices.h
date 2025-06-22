#pragma once
#include "BasicConsole/BasicConsole.h"
#include "Paging/PageFrameAllocator/PageFrameAllocator.h"
#include "Paging/PageTableManager/PageTableManager.h"
#include "GDT/GDT.h"
#include "IDT/IDT.h"
#include "APIC/APIC.h"

struct BootInfo {
	FrameBuffer* pFramebuffer;
	EFI_MEMORY_DESCRIPTOR* mMap;
	uint64_t mMapSize;
	uint64_t mMapDescSize;
};

class KernelServices {
public:
	KernelServices(BootInfo* bootinfo);
	
	BasicConsole basicConsole;
	PageFrameAllocator pageFrameAllocator;
	PageTableManager pageTableManager;
	PageTable* PML4;
	GDT gdt;
	IDT idt;
	APIC apic;
};

extern KernelServices* ks;