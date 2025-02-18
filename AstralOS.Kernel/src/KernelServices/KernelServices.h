#pragma once
#include "BasicConsole/BasicConsole.h"
#include "Paging/PageFrameAllocator/PageFrameAllocator.h"
#include "Paging/PageTableManager/PageTableManager.h"

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
	PageTableManager* pageTableManager;

private:
	PageTable* PML4;
};

