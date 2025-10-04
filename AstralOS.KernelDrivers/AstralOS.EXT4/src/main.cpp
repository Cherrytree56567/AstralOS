#define DRIVER
#define FILE
#include <new>
#include "GenericEXT4/GenericEXT4.h"
#include "global.h"

DriverServices* g_ds = nullptr;

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

extern "C"
DriverInfo DriverMain(DriverServices& DServices) {
    g_ds = &DServices;

    DriverInfo di((char*)"", 1, 0, 0);
    di.name = DServices.strdup("EXT4 Driver");

    void* mem = DServices.malloc(sizeof(GenericEXT4));
    if (!mem) {
        DServices.Println("Failed to Malloc for Generic EXT4");
        DServices.Println(to_hstring((uint64_t)mem));
        di.exCode = 1;
        return di;
    }

    GenericEXT4* factory = new(mem) GenericEXT4();

    DServices.RegisterDriver(factory);

    return di;
}