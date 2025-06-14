#include "PageTableManager.h"

void PageTableManager::Initialize(PageTable* PML4Address, PageFrameAllocator *pfa, BasicConsole* console) {
    this->PML4 = PML4Address;
    this->pageFrameAlloc = pfa;
    this->basicConsole = console;
    this->initialized = true;
}

void PageTableManager::MapMemory(void* virtualMemory, void* physicalMemory) {
    if (!initialized) {
        basicConsole->Println("PageTableManager not initialized, cannot map memory.");
        basicConsole->Println("Did you call Initialize()?");
        return;
    }

    PageMapIndexer indexer = PageMapIndexer((uint64_t)virtualMemory);
    PageTableEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    PageTable* PDP;
    if (!PDE.Present) {
        PDP = (PageTable*)pageFrameAlloc->RequestPage();
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
        PD = (PageTable*)pageFrameAlloc->RequestPage();
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
        PT = (PageTable*)pageFrameAlloc->RequestPage();
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