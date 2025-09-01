#pragma once
#include <cstdint>
#include "../../KernelServices/Paging/MemoryAlloc/Heap.h"
#include "../cstr/cstr.h"
#include "../utils.h"

void Print(const char* str);

/*
 * Code from the Linux Kernel
 * and from KPP:
 * https://github.com/orafaelmiguel/kpp-lib/blob/main/include/kpp/util/List.hpp
*/

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define list_for_each_safe(pos, tmp, head) \
    for (pos = (head)->next, tmp = pos->next; pos != (head); pos = tmp, tmp = pos->next)

struct list_head {
    struct list_head* next;
    struct list_head* prev;
};

inline void INIT_LIST_HEAD(struct list_head* list) {
    list->next = list;
    list->prev = list;
}

inline bool list_empty(const struct list_head* head) {
    return head->next == head;
}

inline void list_add(struct list_head* new_node, struct list_head* head) {
    new_node->next = head->next;
    new_node->prev = head;
    head->next->prev = new_node;
    head->next = new_node;
}

inline void list_add_tail(struct list_head* new_node, struct list_head* head) {
    new_node->next = head;
    new_node->prev = head->prev;
    head->prev->next = new_node;
    head->prev = new_node;
}

inline void list_del(struct list_head* entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->next = entry->prev = nullptr;
}

class List {
public:
    class Iterator
    {
    private:
        struct list_head* node_;
    public:
        explicit Iterator(struct list_head* node) : node_(node) {}

        struct list_head* operator*() const { return node_; }
        Iterator& operator++() { node_ = node_->next; return *this; }
        bool operator!=(const Iterator& other) const { return node_ != other.node_; }
    };

    struct list_head head_;

public:
    List()
    {
        INIT_LIST_HEAD(&head_);
    }

    ~List() = default;

    List(const List&) = delete;
    List& operator=(const List&) = delete;
    List(List&&) = delete;
    List& operator=(List&&) = delete;

    [[nodiscard]] bool empty() const
    {
        return list_empty(&head_);
    }

    void add(struct list_head* node)
    {
        list_add(node, &head_);
    }

    void add_tail(struct list_head* node)
    {
        list_add_tail(node, &head_);
    }

    static void del(struct list_head* node)
    {
        list_del(node);
    }

    [[nodiscard]] Iterator begin() const
    {
        return Iterator(head_.next);
    }

    [[nodiscard]] Iterator end() const
    {
        return Iterator(const_cast<struct list_head*>(&head_));
    }
};