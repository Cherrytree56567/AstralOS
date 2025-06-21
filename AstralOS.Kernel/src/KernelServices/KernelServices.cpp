#include "KernelServices.h"

KernelServices* ks;

KernelServices::KernelServices(BootInfo* bootinfo) : basicConsole(bootinfo->pFramebuffer) {
	pageFrameAllocator.Initialise(&basicConsole);
	gdt.Initialize(&basicConsole);
	ks = this;
	idt.Initialize(&basicConsole);
}