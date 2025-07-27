#pragma once
#include <cstdint>
#include "../../KernelServices/Paging/MemoryAlloc/Heap.h"
#include "../cstr/cstr.h"

void Print(const char* str);

template<typename T>
struct Array {
    T* data = nullptr;
    size_t size = 0;
    size_t capacity = 0;

    Array() {
        size = 0;
        capacity = 0;
    }

    ~Array() {
        if (data) free(data);
        data = nullptr;
        size = 0;
        capacity = 0;
    }

    void push_back(const T& value) {
        if (size >= capacity) {
            grow();
        }
        if (data) {
            data[size++] = value;
        } else {
            Print("OUT OF MEM!");
        }
    }

    void grow() {
        size_t newCapacity = (capacity == 0) ? 16 : capacity * 2;
        T* newData = (T*)malloc(sizeof(T) * newCapacity);
        if (!newData) {
            Print("ALLOCATION FAILURE");
            return;
        }
        for (size_t i = 0; i < size; ++i) {
            newData[i] = data[i];
        }
        if (data) free(data);
        data = newData;
        capacity = newCapacity;
    }

    void clear() {
        size = 0;
    }

    T& operator[](size_t index) {
        return data[index];
    }

    const T& operator[](size_t i) const { 
        return data[i]; 
    }
};