#pragma once
#include "../Paging/MemoryAlloc/Heap.h"
#include <cstdint>

template<typename T>
struct Array {
    T* data = nullptr;
    size_t size = 0;
    size_t capacity = 0;

    void push_back(const T& value) {
        if (size >= capacity) {
            grow();
        }
        data[size++] = value;
    }

    void grow() {
        capacity = (capacity == 0) ? 16 : capacity * 2;
        T* newData = (T*)malloc(sizeof(T) * capacity);
        for (size_t i = 0; i < size; ++i) {
            newData[i] = data[i];
        }
        if (data) free(data);
        data = newData;
    }

    void clear() {
        size = 0;
    }

    T& operator[](size_t index) {
        return data[index];
    }
};
