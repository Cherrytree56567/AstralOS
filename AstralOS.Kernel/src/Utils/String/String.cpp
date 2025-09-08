#include "String.h"

String::String(const char* str) {
    size_t len = strlen(str);
    data = new char[len + 1];
    strcpy(data, str);
}

String String::operator+(const String& other) const {
    size_t len = strlen(data) + strlen(other.data);
    char* newData = new char[len + 1];
    strcpy(newData, data);
    strcat(newData, other.data);
    return String(newData);
}