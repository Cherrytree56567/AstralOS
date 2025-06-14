#include "KernelServices.h"

KernelServices::KernelServices(BootInfo* bootinfo) : basicConsole(BasicConsole(bootinfo->pFramebuffer)) {
	pageFrameAllocator.Initialise(&basicConsole);
}