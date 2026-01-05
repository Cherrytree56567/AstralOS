#pragma once
#include <cstdint>
#include "../utils.h"

/*
 * Sorry, but its GPT Generated, I will get back to this later
*/
class String {
public:
    String(const char* str);
    String(const char* buf, size_t length);

    String operator+(const String& other) const;

    bool StartsWith(const String& prefix) const;
    uint64_t size() const;
    String substring(size_t start) const;
    String substring(size_t start, size_t length) const;

    const char* c_str() const {
        return data;
    }
private:
    char* data;
};