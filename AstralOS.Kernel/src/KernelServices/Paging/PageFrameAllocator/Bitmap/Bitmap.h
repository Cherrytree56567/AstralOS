#pragma once
#include <stddef.h>
#include <stdint.h>

struct Bitmap {
    bool operator[](uint64_t index);
    bool Set(uint64_t index, bool value);
    uint8_t* buffer;
    size_t size;
};