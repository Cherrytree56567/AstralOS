#pragma once
#include <cstdint>
#include <new>
#include "../../KernelServices/Paging/MemoryAlloc/Heap.h"
#include "../cstr/cstr.h"
#include "../utils.h"

void Print(const char* str);

struct Node {
    uintptr_t data;
    uintptr_t next;
};

/*
 * Not AI anymore.
 * 
 * This array is an example of
 * a linked list. It makes use
 * of 2 uintptr_t' to keep the
 * malloc'ed data and the next
 * malloc'ed Node struct.
 * 
 * As for the operator[] funcs
 * they store a pointer to the
 * current Node and loops over
 * the next pointer in the node
 * and then returns it.
*/
template<typename T>
struct Array {
    Array() = default;

    ~Array() { clear(); }

    void push_back(const T& value) {
        struct Node *node = (struct Node*)malloc(sizeof(struct Node));
        if (!node) {
            return;
        }

        void *data_ptr = malloc(sizeof(T));
        if (!data_ptr) {
            free(node);
            return;
        }
        memcpy(data_ptr, &value, sizeof(T));

        node->data = (uintptr_t)data_ptr;
        node->next = 0;

        if (head == nullptr) {
            head = node;
        } else {
            Node* current = head;
            while (current->next != 0) {
                current = (Node*)current->next;
            }
            current->next = (uintptr_t)node;
        }
        _size++;
    }

    size_t size() const { return _size; }

    T& operator[](size_t index) {
        Node* current = head;
        for (size_t i = 0; i < index; i++) {
            current = (Node*)current->next;
        }
        return *(T*)current->data;
    }

    const T& operator[](size_t index) const {
        Node* current = head;
        for (size_t i = 0; i < index; ++i) {
            current = (Node*)current->next;
        }
        return *(T*)current->data;
    }

    void clear() {
        Node* current = head;
        while (current) {
            Node* next = (Node*)current->next;
            free((void*)current->data);
            free((void*)current);
            current = next;
        }
        head = nullptr;
        _size = 0;
    }

private:
    Node* head = nullptr;
    size_t _size = 0;
};
