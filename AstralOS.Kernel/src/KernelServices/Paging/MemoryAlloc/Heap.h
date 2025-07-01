#pragma once
#include <cstdint>
#include <cstddef>

/*
 * We need a Block Header to
 * know if there is any free
 * mem and to free any alloc
 * mem.
*/
struct BlockHeader;

struct BlockHeader {
    size_t size;
    bool free;
    BlockHeader* next;
    BlockHeader* prev;
};

class HeapAllocator {
public:
    HeapAllocator() {}

    void Initialize();

    void* malloc(size_t size);
    void free(void* ptr);
private:
    uint64_t heapStart;
    uint64_t heapEnd;
    uint64_t heapCurrent;
    BlockHeader* hdr;
};