#pragma once
#include "main.h"
#include <stdbool.h>

#define KERNEL_VIRT_ADDR 0xFFFFFFFF80000000ULL
#define HIGHER_VIRT_ADDR 0xFFFFFFFF00000000

/*
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-9-Page-Table-Manager/kernel/src/paging/paging.h
*/
enum PT_Flag {
    Present = 0,
    ReadWrite = 1,
    UserSuper = 2,
    WriteThrough = 3,
    CacheDisabled = 4,
    Accessed = 5,
    LargerPages = 7,
    Custom0 = 9,
    Custom1 = 10,
    Custom2 = 11,
    NX = 63 // only if supported
};

struct PageDirectoryEntry {
    uint64_t Value;
};

struct PageDirectoryEntry SetFlag(struct PageDirectoryEntry pd, enum PT_Flag flag, bool enabled);
bool GetFlag(struct PageDirectoryEntry pd, enum PT_Flag flag);
struct PageDirectoryEntry SetAddress(struct PageDirectoryEntry pd, uint64_t address);
uint64_t GetAddress();

struct PageTable { 
    struct PageDirectoryEntry entries [512];
} __attribute__((aligned(0x1000)));

struct PageMapIndexer {
    uint64_t PDP_i;
    uint64_t PD_i;
    uint64_t PT_i;
    uint64_t P_i;
};

struct PageMapIndexer CreatePageMapIndexer(uint64_t virtualAddress);

static struct PageTable* PML4;

void InitialisePaging();
void MapMemory(void* virtualMemory, void* physicalMemory);
struct PageTable* GetNextTable(struct PageTable* table, uint64_t index);