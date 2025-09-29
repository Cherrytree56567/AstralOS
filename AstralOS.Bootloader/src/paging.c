#include "paging.h"
#include <stdbool.h>
#include <string.h>
#include "pfa.h"

struct PageTable* GetNextTable(struct PageTable* table, uint64_t index) {
    if (!(table->entries[index].Value & 1)) return NULL;
    return (struct PageTable*)(table->entries[index].Value & ~0xFFFULL);
}

struct PageMapIndexer CreatePageMapIndexer(uint64_t virtualAddress) {
    struct PageMapIndexer pmi;
    virtualAddress >>= 12;
    pmi.P_i = virtualAddress & 0x1ff;
    virtualAddress >>= 9;
    pmi.PT_i = virtualAddress & 0x1ff;
    virtualAddress >>= 9;
    pmi.PD_i = virtualAddress & 0x1ff;
    virtualAddress >>= 9;
    pmi.PDP_i = virtualAddress & 0x1ff;

    return pmi;
}

struct PageDirectoryEntry SetFlag(struct PageDirectoryEntry pd, enum PT_Flag flag, bool enabled) {
    uint64_t bitSelector = (uint64_t)1 << flag;
    pd.Value &= ~bitSelector;
    if (enabled){
        pd.Value |= bitSelector;
    }
    return pd;
}

bool GetFlag(struct PageDirectoryEntry pd, enum PT_Flag flag) {
    uint64_t bitSelector = (uint64_t)1 << flag;
    return pd.Value & bitSelector > 0 ? true : false;
}

uint64_t GetAddress(struct PageDirectoryEntry pd) {
    return (pd.Value & 0x000ffffffffff000) >> 12;
}

struct PageDirectoryEntry SetAddress(struct PageDirectoryEntry pd, uint64_t address) {
    address &= 0x000000ffffffffff;
    pd.Value &= 0xfff0000000000fff;
    pd.Value |= (address << 12);
    return pd;
}

void MapMemory(void* virtualMemory, void* physicalMemory) {
    struct PageMapIndexer indexer = CreatePageMapIndexer((uint64_t)virtualMemory);
    struct PageDirectoryEntry PDE;

    bool isPDPRequested = false;
    bool isPDRequested = false;
    bool isPTRequested = false;

    PDE = PML4->entries[indexer.PDP_i];
    struct PageTable* PDP;
    if (!GetFlag(PDE, Present)) {
        PDP = (struct PageTable*)RequestPage();
        memset(PDP, 0, 0x1000);
        PDE = SetAddress(PDE, (uint64_t)PDP >> 12);
        PDE = SetFlag(PDE, Present, true);
        PDE = SetFlag(PDE, ReadWrite, true);
        PML4->entries[indexer.PDP_i] = PDE;
        isPDPRequested = true;
    } else {
        PDP = (struct PageTable*)((uint64_t)GetAddress(PDE) << 12);
    }

    Print(L"PDP: 0x%lx\n", (uint64_t)PDP);

    PDE = PDP->entries[indexer.PD_i];
    struct PageTable* PD;
    if (!GetFlag(PDE, Present)) {
        PD = (struct PageTable*)RequestPage();
        memset(PD, 0, 0x1000);
        PDE = SetAddress(PDE, (uint64_t)PD >> 12);
        PDE = SetFlag(PDE, Present, true);
        PDE = SetFlag(PDE, ReadWrite, true);
        PDP->entries[indexer.PD_i] = PDE;
        isPDRequested = true;
    } else {
        PD = (struct PageTable*)((uint64_t)GetAddress(PDE) << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    struct PageTable* PT;
    if (!GetFlag(PDE, Present)) {
        PT = (struct PageTable*)RequestPage();
        memset(PT, 0, 0x1000);
        PDE = SetAddress(PDE, (uint64_t)PT >> 12);
        PDE = SetFlag(PDE, Present, true);
        PDE = SetFlag(PDE, ReadWrite, true);
        PD->entries[indexer.PT_i] = PDE;
        isPTRequested = true;
    } else {
        PT = (struct PageTable*)((uint64_t)GetAddress(PDE) << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE = SetAddress(PDE, (uint64_t)physicalMemory >> 12);
    PDE = SetFlag(PDE, Present, true);
    PDE = SetFlag(PDE, ReadWrite, true);
    PDE = SetFlag(PDE, WriteThrough, false);
    PDE = SetFlag(PDE, CacheDisabled, false);
    PT->entries[indexer.P_i] = PDE;
}

void InitialisePaging() {
    if (PML4 != NULL) {
        return;
    }

    uint64_t IDENTITY_BYTES = 2 * 1024 * 1024;

    Print(L"Initialising Paging...\n");
    
    uint64_t pml4_phys = (uint64_t)RequestPage();
    Print(L"Requested Page for PML4: 0x%lx\n", pml4_phys);
    if (pml4_phys == 0) {
        Print(L"Failed to Request Page for the PML4");
        return;
    }

    PML4 = (struct PageTable*)(uintptr_t)pml4_phys;

    memset(PML4, 0, PAGE_SIZE);

    Print(L"PML4 Address: 0x%lx\n", (uint64_t)PML4);

    for (uint64_t off = 0; off < IDENTITY_BYTES; off += PAGE_SIZE) {
        void* va = (void*)(uintptr_t)off;
        void* pa = (void*)(uintptr_t)off;
        MapMemory(va, pa);
    }
}