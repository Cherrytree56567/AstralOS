#pragma once
#include <cstdint>
#include "../../Utils/Array/Array.h"

struct ThreadContext {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;

    uint64_t rip;
    uint64_t rflags;

    uint64_t cs, ds, es, fs, gs, ss;

    uint8_t xmm[16 * 16];
    uint8_t ymm[16 * 16];

    uint8_t opmask[8];
    uint8_t zmm[16 * 64];

    uint8_t reserved[64];

    uint64_t kernel_rsp;

    uint8_t* stackBase;
    size_t stackSize;

    enum State { Running, Ready, Sleeping, Blocked, Terminated } state;

    void (*func)(void*);
    void* arg;
};

class Threading {
public:
    void Initialize();

    uint64_t AddThread(void (*func)(void*), void* arg);
    void RemoveThread(uint64_t thread);

private:
    Array<ThreadContext> threads;
};