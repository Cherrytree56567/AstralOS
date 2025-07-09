#pragma once
#include "../../Paging/Paging.h"
#include "../../Paging/PageFrameAllocator/PageFrameAllocator.h"
#include "../../BasicConsole/BasicConsole.h"
#include "../../../cstr/cstr.h"
#include "PageMapIndexer/PageMapIndexer.h"

/*
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-8-Page-Table-Manager/kernel/src/paging/PageTableManager.h
*/
class PageTableManager {
public:
    PageTableManager() {}
    void MapMemory(void* virtualMemory, void* physicalMemory, bool cache = true);
    void Initialize(PageTable* PML4Address, PageFrameAllocator *pfa, BasicConsole* console);
    void UnmapMemory(void* virtualMemory);

private:
	PageFrameAllocator* pageFrameAlloc;
    PageTable* PML4;
    BasicConsole* basicConsole;
    bool initialized = false;

    PageTable* GetNextTable(PageTable* table, uint64_t index);
};