#pragma once
#include "BasicConsole/BasicConsole.h"
#include "Paging/PageFrameAllocator/PageFrameAllocator.h"
#include "Paging/PageTableManager/PageTableManager.h"
#include "GDT/GDT.h"
#include "IDT/IDT.h"
#include "APIC/APIC.h"
#include "PIC/PIC.h"
#include "IOAPIC/IOAPIC.h"
#include "ACPI/ACPI.h"
#include "Paging/MemoryAlloc/Heap.h"
#include "PCI/PCI.h"
#include "PCIe/PCIe.h"
#include "InitialRamFS/InitialRamFS.h"
#include "DriverManager/DriverManager.h"
#include "Timer/Timer.h"

struct BootInfo {
	FrameBuffer pFramebuffer;
	EFI_MEMORY_DESCRIPTOR* mMap;
	uint64_t mMapSize;
	uint64_t mMapDescSize;
	void* rsdp;
    void* initrdBase;
    uint64_t initrdSize;
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
	PIC pic;
	IOAPIC ioapic;
	ACPI acpi;
	HeapAllocator heapAllocator;
	PCI pci;
	PCIe pcie;
	InitialRamFS initram;
	BootInfo pBootInfo;
	DriverManager driverMan;
	APICTimer timer;
	PIT pit;
};

extern KernelServices* ks;