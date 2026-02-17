#include "Threading.h"

void Threading::Initialize() {
    
}

uint64_t Threading::AddThread(void (*func)(void*), void* arg) {
    ThreadContext ctx;
    ctx.func = func;
    ctx.arg = arg;

    threads.push_back(ctx);
    return threads.size() - 1;
}

void Threading::RemoveThread(uint64_t thread) {
    
}