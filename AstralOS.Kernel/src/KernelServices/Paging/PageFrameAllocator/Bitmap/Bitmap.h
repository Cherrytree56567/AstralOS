#pragma once
#include <stddef.h>
#include <stdint.h>

// 0 - Page is Free
// 1 - Page is Reserved
enum BitmapStatus {
	Free = false,
	Reserved = true
};

struct Bitmap {
    bool operator[](uint64_t index);
    void Set(uint64_t index, bool value);
    uint8_t* buffer;
    size_t size;
};