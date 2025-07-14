#include "Bitmap.h"
#include "../../../KernelServices.h"

bool Bitmap::operator[](uint64_t index) {
    if (index >= size * 8) {
        ks->basicConsole.Print("Bitmap OOB read at index: ");
        ks->basicConsole.Println(to_hstring(index));
        return false;
    }
    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;
    if ((buffer[byteIndex] & bitIndexer) > 0) {
        return true;
    }
    return false;
}

bool Bitmap::Set(uint64_t index, bool value) {
    if (index >= size * 8) {
        ks->basicConsole.Print("Bitmap OOB read at index: ");
        ks->basicConsole.Println(to_hstring(index));
        return false;
    }
    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;
    buffer[byteIndex] &= ~bitIndexer;
    if (value) {
        buffer[byteIndex] |= bitIndexer;
    }
    return true;
}