#include "PageTableManager.h"

void PageTableManager::Initialize(PageTable* PML4Address, PageFrameAllocator *pfa, BasicConsole* console) {
    this->PML4 = PML4Address;
    this->pageFrameAlloc = pfa;
    this->basicConsole = console;
    this->initialized = true;
}

/*
 * TODO: GOTCHA:
 * Map this when you switch to higher half,
 * otherwise spend countless hours debugging this.
*/

void PageTableManager::MapMemory(void* virtualMemory, void* physicalMemory, bool cache) {
    if (!initialized) {
        basicConsole->Println("PageTableManager not initialized, cannot map memory.");
        basicConsole->Println("Did you call Initialize()?");
        return;
    }

    PageMapIndexer indexer = PageMapIndexer((uint64_t)virtualMemory);
    PageDirectoryEntry PDE;

    bool isPDPRequested = false;
    bool isPDRequested = false;
    bool isPTRequested = false;

    PDE = PML4->entries[indexer.PDP_i];
    PageTable* PDP;
    if (!PDE.GetFlag(PT_Flag::Present)) {
        PDP = (PageTable*)pageFrameAlloc->RequestPage();
        memset(PDP, 0, 0x1000);
        PDE.SetAddress((uint64_t)PDP >> 12);
        PDE.SetFlag(PT_Flag::Present, true);
        PDE.SetFlag(PT_Flag::ReadWrite, true);
        PML4->entries[indexer.PDP_i] = PDE;
        isPDPRequested = true;
    }
    else
    {
        PDP = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }


    PDE = PDP->entries[indexer.PD_i];
    PageTable* PD;
    if (!PDE.GetFlag(PT_Flag::Present)) {
        PD = (PageTable*)pageFrameAlloc->RequestPage();
        memset(PD, 0, 0x1000);
        PDE.SetAddress((uint64_t)PD >> 12);
        PDE.SetFlag(PT_Flag::Present, true);
        PDE.SetFlag(PT_Flag::ReadWrite, true);
        PDP->entries[indexer.PD_i] = PDE;
        isPDRequested = true;
    }
    else
    {
        PD = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    PageTable* PT;
    if (!PDE.GetFlag(PT_Flag::Present)) {
        PT = (PageTable*)pageFrameAlloc->RequestPage();
        memset(PT, 0, 0x1000);
        PDE.SetAddress((uint64_t)PT >> 12);
        PDE.SetFlag(PT_Flag::Present, true);
        PDE.SetFlag(PT_Flag::ReadWrite, true);
        PD->entries[indexer.PT_i] = PDE;
        isPTRequested = true;
    }
    else
    {
        PT = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE.SetAddress((uint64_t)physicalMemory >> 12);
    PDE.SetFlag(PT_Flag::Present, true);
    PDE.SetFlag(PT_Flag::ReadWrite, true);
    if (cache) {
        PDE.SetFlag(PT_Flag::WriteThrough, false);
        PDE.SetFlag(PT_Flag::CacheDisabled, false);
    } else {
        PDE.SetFlag(PT_Flag::WriteThrough, true);
        PDE.SetFlag(PT_Flag::CacheDisabled, true);
    }
    PT->entries[indexer.P_i] = PDE;
}

void PageTableManager::UnmapMemory(void* virtualMemory) {
    if (!initialized) {
        basicConsole->Println("PageTableManager not initialized, cannot unmap memory.");
        return;
    }

    PageMapIndexer indexer = PageMapIndexer((uint64_t)virtualMemory);
    
    PageDirectoryEntry PDE = PML4->entries[indexer.PDP_i];
    if (!PDE.GetFlag(PT_Flag::Present)) return;

    PageTable* PDP = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    PDE = PDP->entries[indexer.PD_i];
    if (!PDE.GetFlag(PT_Flag::Present)) return;

    PageTable* PD = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    PDE = PD->entries[indexer.PT_i];
    if (!PDE.GetFlag(PT_Flag::Present)) return;

    PageTable* PT = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
    PDE = PT->entries[indexer.P_i];

    if (!PDE.GetFlag(PT_Flag::Present)) return;

    PDE.Value &= ~((uint64_t)PT_Flag::Present);
    PT->entries[indexer.P_i] = PDE;

    asm volatile("invlpg (%0)" : : "r"(virtualMemory) : "memory");
}

PageTable* PageTableManager::GetNextTable(PageTable* table, uint64_t index) {
    if (!(table->entries[index].Value & 1)) return nullptr;
    return (PageTable*)(table->entries[index].Value & ~0xFFFULL);
}
