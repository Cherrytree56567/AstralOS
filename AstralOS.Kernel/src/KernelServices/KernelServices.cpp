#include "KernelServices.h"

KernelServices::KernelServices(BootInfo* bootinfo) : basicConsole(BasicConsole(bootinfo->pFramebuffer)) {
	basicConsole.Println("TEP1");
	PageTable PM = *(PageTable*)pageFrameAllocator.RequestPage();
	PML4 = &PM;
	basicConsole.Println("TEP1.5");
	memset(PML4, 0, 0x1000);
	basicConsole.Println("TEP2");
	PageTableManager pf(PML4, &pageFrameAllocator);
	basicConsole.Println("TEP3");
	pageTableManager = &pf;
	basicConsole.Println("TEP4");
}