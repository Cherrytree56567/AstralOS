#include "PageTableManager.h"

PageTableManager::PageTableManager(PageTable* PML4Address, PageFrameAllocator *pfa) {
    this->PML4 = PML4Address;
	this->pfa = pfa;
}

void PageTableManager::MapMemory(void* virtualMemory, void* physicalMemory) {
    PageMapIndexer indexer = PageMapIndexer((uint64_t)virtualMemory);
    PageTableEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    PageTable* PDP;
    if (!PDE.Present) {
        PDP = (PageTable*)pfa->RequestPage();
        memset(PDP, 0, 0x1000);
        PDE.Address = (uint64_t)PDP >> 12;
        PDE.Present = true;
        PDE.ReadWrite = true;
        PML4->entries[indexer.PDP_i] = PDE;
    }
    else
    {
        PDP = (PageTable*)((uint64_t)PDE.Address << 12);
    }


    PDE = PDP->entries[indexer.PD_i];
    PageTable* PD;
    if (!PDE.Present) {
        PD = (PageTable*)pfa->RequestPage();
        memset(PD, 0, 0x1000);
        PDE.Address = (uint64_t)PD >> 12;
        PDE.Present = true;
        PDE.ReadWrite = true;
        PDP->entries[indexer.PD_i] = PDE;
    }
    else
    {
        PD = (PageTable*)((uint64_t)PDE.Address << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    PageTable* PT;
    if (!PDE.Present) {
        PT = (PageTable*)pfa->RequestPage();
        memset(PT, 0, 0x1000);
        PDE.Address = (uint64_t)PT >> 12;
        PDE.Present = true;
        PDE.ReadWrite = true;
        PD->entries[indexer.PT_i] = PDE;
    }
    else
    {
        PT = (PageTable*)((uint64_t)PDE.Address << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE.Address = (uint64_t)physicalMemory >> 12;
    PDE.Present = true;
    PDE.ReadWrite = true;
    PT->entries[indexer.P_i] = PDE;
}