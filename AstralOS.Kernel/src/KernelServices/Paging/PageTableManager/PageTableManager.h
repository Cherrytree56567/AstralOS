#pragma once
#include "../../Paging/Paging.h"
#include "../../Paging/PageFrameAllocator/PageFrameAllocator.h"
#include "PageMapIndexer/PageMapIndexer.h"

/*
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-8-Page-Table-Manager/kernel/src/paging/PageTableManager.h
*/
class PageTableManager {
public:
    PageTableManager(PageTable* PML4Address, PageFrameAllocator *pfa);
    PageTable* PML4;
    void MapMemory(void* virtualMemory, void* physicalMemory);

private:
	PageFrameAllocator* pfa;
};