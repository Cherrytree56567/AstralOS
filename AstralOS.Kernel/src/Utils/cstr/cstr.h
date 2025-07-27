#pragma once
#include <stdint.h>

/*
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-4-C%2B%2B-Number-To-String/kernel/src/cstr.h
*/
const char* to_string(uint64_t value);
const char* to_string(int64_t value);
const char* to_hstring(uint64_t value);
const char* to_hstring(uint32_t value);
const char* to_hstring(uint16_t value);
const char* to_hstring(uint8_t value);
const char* to_string(double value, uint8_t decimalPlaces);
const char* to_string(double value);