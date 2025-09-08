#pragma once
#include <cstdint>
#include "../utils.h"

/*
 * Sorry, but its GPT Generated, I will get back to this later
*/
class String {
public:
    String(const char* str);

    String operator+(const String& other) const;

    const char* c_str() const {
        return data;
    }
private:
    char* data;
};