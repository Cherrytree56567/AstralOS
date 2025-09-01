#pragma once
#include <cstdint>
#include <cstddef>

int strcmp(const char* s1, const char* s2);
int memcmp(const void* s1, const void* s2, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
char* strncpy(char* dest, const char* src, size_t maxLen);
int strncmp(const char* s1, const char* s2, size_t n);
size_t strlen(const char* str);
char* strchr(const char* str, int ch);
char* strdup(const char* src);

void* operator new(size_t size);
void operator delete(void* ptr) noexcept;
void* operator new[](size_t size);
void operator delete[](void* ptr) noexcept;
