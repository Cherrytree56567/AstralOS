#include "String.h"

String::String(const char* str) {
    size_t len = strlen(str);
    data = new char[len + 1];
    strcpy(data, str);
}

String::String(const char* buf, size_t length) {
    data = new char[length + 1];

    if (buf && length > 0)
        memcpy(data, buf, length);

    data[length] = '\0';
}

String String::operator+(const String& other) const {
    size_t len = strlen(data) + strlen(other.data);
    char* newData = new char[len + 1];
    strcpy(newData, data);
    strcat(newData, other.data);
    return String(newData);
}

bool String::StartsWith(const String& prefix) const {
    if (size() < prefix.size())
        return false;

    return memcmp(data, prefix.c_str(), prefix.size()) == 0;
}

uint64_t String::size() const {
    return strlen(data);
}

String String::substring(size_t start) const {
    if (start >= size())
        return String("");

    return String(data + start, size() - start);
}

String String::substring(size_t start, size_t length) const {
    if (start >= size())
        return String("");

    if (start + length > size())
        length = size() - start;

    return String(data + start, length);
}