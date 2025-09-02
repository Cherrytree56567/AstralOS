#include "Heap.h"
#include "../../KernelServices.h"

/*
 * Initialize the Heap Allocator
 *
 * The Heap Allocator will allow the dev to
 * access any *specific* amount of memory,
 * for example, if the dev needs 10 bytes
 * of mem, they can use the heap allocator
 * instead of allocating a 4KB page, each
 * time.
 * 
 * The Heap Allocator will request a 4KB page
 * to use initially. When it is hungry for more,
 * more pages will be given to it. The heapStart
 * tracks the addr of the heap. The heapEnd will
 * track the end of the heap. The heapCurrent
 * will allow the heap alloc to know where to
 * put the next data using malloc.
 * 
 * We don't need to map the memory, bc the
 * memory is already identity mapped.
 * 
 * Now, since we are using a higher half kernel
 * we need to map it since we un mapped it.
 * 
 * Linux usually allocates its heap at 0xFFFF880000000000
 * so we should be good.
*/
void HeapAllocator::Initialize() {
    void* heapStartPtr = ks->pageFrameAllocator.RequestPage();
    if (!heapStartPtr) {
        ks->basicConsole.Println("Failed to Request Page for Heap Allocator.");
        return;
    }    

    ks->pageTableManager.MapMemory((void*)0xFFFF880000000000, heapStartPtr);

    heapStart = 0xFFFF880000000000;
    heapEnd = heapStart + PAGE_SIZE;
    heapCurrent = heapStart;

    /*
     * bc we want to free mem in our
     * heap and reuse that mem, we 
     * need to keep track of the mem
     * we freed.
    */
    hdr = (BlockHeader*)heapStart;
    hdr->size = PAGE_SIZE - sizeof(BlockHeader);
    hdr->free = true;
    hdr->next = nullptr;
    hdr->prev = nullptr;
}

/*
 * malloc()
 * Standard function to allocate a specific
 * amount of memory.
 * 
 * -- How it works --
 * We need to check if the size is null, so
 * that we don't waste our time.
 * 
 * We should Align the size, so that we can
 * do our math faster.
 * 
 * If our current header is free, and the
 * heap size is bigger (inclusive) than the 
 * size we need to allocate, then we can
 * create another block and initialize it.
 * 
 * If we don't have any block's left to use
 * then we can request for more pages, 
 * extend the heapEnd, add more blocks and
 * try again.
*/
void* HeapAllocator::malloc(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    size = (size + 7) & ~7;

    BlockHeader* current = hdr;

    while (current) {
        if (current->free && current->size >= size) {
            if (current->size >= size + sizeof(BlockHeader) + 8) {
                BlockHeader* newBlock = (BlockHeader*)((uint8_t*)current + sizeof(BlockHeader) + size);
                newBlock->size = current->size - size - sizeof(BlockHeader);
                newBlock->free = true;
                newBlock->next = current->next;
                newBlock->prev = current;
                if (newBlock->next)
                    newBlock->next->prev = newBlock;

                current->next = newBlock;
                current->size = size;
            }

            current->free = false;
            return (void*)((uint8_t*)current + sizeof(BlockHeader));
        }
        current = current->next;
    }
    
    // Dont forget to map.
    void* physPage = ks->pageFrameAllocator.RequestPage();
    if (!physPage) {
        return nullptr;
    }
    ks->pageTableManager.MapMemory((void*)heapEnd, physPage);

    BlockHeader* newBlock = (BlockHeader*)(heapEnd);
    newBlock->size = PAGE_SIZE - sizeof(BlockHeader);
    newBlock->free = false;
    newBlock->next = nullptr;
    newBlock->prev = nullptr;

    if (!hdr) {
        hdr = newBlock;
    } else {
        BlockHeader* tail = hdr;
        while (tail->next) tail = tail->next;
        tail->next = newBlock;
        newBlock->prev = tail;
    }

    heapEnd += PAGE_SIZE;

    if (newBlock->size >= size) {
        return (void*)((uint8_t*)newBlock + sizeof(BlockHeader));
    }
    
    return nullptr;
}

/*
 * free()
 * Standard function to de-allocate a
 * specific part of the heap.
 * 
 * -- How it works --
 * If the pointer is invalid, we must
 * not waste our time.
 * 
 * Find the block we need to free and
 * set the block to free.
 * 
 * We can then check if the block
 * next to us or behind us is free so
 * that we can merge those together.
*/
void HeapAllocator::free(void* ptr) {
    if (!ptr) {
        return;
    }

    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    block->free = true;

    if (block->next && block->next->free) {
        block->size += sizeof(BlockHeader) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->free) {
        block->prev->size += sizeof(BlockHeader) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/*
 * We can use this func to easily malloc
 * without having to pass the KernelServices
 * var pointer.
*/
void* malloc(size_t size) {
    return ks->heapAllocator.malloc(size);
}

void free(void* ptr) {
    ks->heapAllocator.free(ptr);
}