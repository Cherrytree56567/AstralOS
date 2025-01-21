#include "cstr.h"

/*
* Convert a UINT64_T to a string
*/
const char* to_string(uint64_t value) {
    static char buffer[21];
    int length = 20;
    buffer[length] = '\0';

    if (value == 0) {
        buffer[--length] = '0';
        return &buffer[length];
    }

    while (value > 0) {
        buffer[--length] = '0' + (value % 10);
        value /= 10;
    }

    return &buffer[length];
}

/*
* Convert a UINT32_T to a string
*/
const char* to_string(int64_t value) {
    static char buffer[21];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    bool isNegative = (value < 0);
    if (isNegative) value = -value;

    do {
        *--ptr = '0' + (value % 10);
        value /= 10;
    } while (value > 0);

    if (isNegative) *--ptr = '-';

    return ptr;
}

/*
* Convert a UINT64_T to a hex string
*/
const char* to_hstring(uint64_t value) {
    static char buffer[19];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    do {
        uint8_t nibble = value & 0xF;
        *--ptr = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    } while (value > 0);

    *--ptr = 'x';
    *--ptr = '0';

    return ptr;
}

/*
* Convert a UINT32_T to a hex string
*/
const char* to_hstring(uint32_t value) {
    static char buffer[11];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    do {
        uint8_t nibble = value & 0xF;
        *--ptr = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    } while (value > 0);

    *--ptr = 'x';
    *--ptr = '0';

    return ptr;
}

/*
* Convert a UINT16_T to a hex string
*/
const char* to_hstring(uint16_t value) {
    static char buffer[7];
    char* ptr = buffer + sizeof(buffer) - 1; 
    *ptr = '\0';

    do {
        uint8_t nibble = value & 0xF;
        *--ptr = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    } while (value > 0);

    *--ptr = 'x';
    *--ptr = '0';

    return ptr;
}

/*
* Convert a UINT8_T to a hex string
*/
const char* to_hstring(uint8_t value) {
    static char buffer[5];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    do {
        uint8_t nibble = value & 0xF;
        *--ptr = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    } while (value > 0);

    *--ptr = 'x';
    *--ptr = '0';

    return ptr;
}

/*
* Convert a double to a string with a specified number of decimal places
*/
const char* to_string(double value, uint8_t decimalPlaces) {
    static char buffer[32];
    char* ptr = buffer;

    if (value < 0) {
        *ptr++ = '-';
        value = -value;
    }

    uint64_t integerPart = static_cast<uint64_t>(value);
    double fractionalPart = value - integerPart;

    char* intEnd = buffer + sizeof(buffer) - 1;
    *intEnd = '\0';
    char* intPtr = intEnd;

    do {
        *--intPtr = '0' + (integerPart % 10);
        integerPart /= 10;
    } while (integerPart > 0);

    while (*intPtr) *ptr++ = *intPtr++;

    *ptr++ = '.';

    for (uint8_t i = 0; i < decimalPlaces; ++i) {
        fractionalPart *= 10;
        uint8_t digit = static_cast<uint8_t>(fractionalPart);
        *ptr++ = '0' + digit;
        fractionalPart -= digit;
    }

    *ptr = '\0';

    return buffer;
}

/*
* Convert a double to a string with 2 decimal places
*/
const char* to_string(double value) {
    static char buffer[32];
    char* ptr = buffer;

    if (value < 0) {
        *ptr++ = '-';
        value = -value;
    }

    uint64_t integerPart = static_cast<uint64_t>(value);
    double fractionalPart = value - integerPart;

    char* intEnd = buffer + sizeof(buffer) - 1;
    *intEnd = '\0';
    char* intPtr = intEnd;

    do {
        *--intPtr = '0' + (integerPart % 10);
        integerPart /= 10;
    } while (integerPart > 0);

    while (*intPtr) *ptr++ = *intPtr++;

    *ptr++ = '.';
    for (int i = 0; i < 2; ++i) {
        fractionalPart *= 10;
        uint8_t digit = static_cast<uint8_t>(fractionalPart);
        *ptr++ = '0' + digit;
        fractionalPart -= digit;
    }

    *ptr = '\0';

    return buffer;
}
