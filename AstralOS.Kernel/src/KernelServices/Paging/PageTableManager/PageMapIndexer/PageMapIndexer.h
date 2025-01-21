#pragma once
#include <stdint.h>

/*
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-8-Page-Table-Manager/kernel/src/paging/PageTableManager.h
*/
class PageMapIndexer {
public:
    PageMapIndexer(uint64_t virtualAddress) {
        virtualAddress >>= 12;
        P_i = virtualAddress & 0x1ff;
        virtualAddress >>= 9;
        PT_i = virtualAddress & 0x1ff;
        virtualAddress >>= 9;
        PD_i = virtualAddress & 0x1ff;
        virtualAddress >>= 9;
        PDP_i = virtualAddress & 0x1ff;
    }
    uint64_t PDP_i;
    uint64_t PD_i;
    uint64_t PT_i;
    uint64_t P_i;
};