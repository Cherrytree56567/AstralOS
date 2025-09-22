#include "GenericIDE.h"
#include "../global.h"

const char* to_hstridng(uint64_t value) {
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

bool GenericIDE::Supports(const DeviceKey& devKey) {
    if (devKey.classCode == 0x01 && devKey.subclass == 0x01) {
        g_ds->Print("Found IDE Device: ");
        g_ds->Print(to_hstridng(devKey.vendorID));
        g_ds->Print(" ");
        g_ds->Print(to_hstridng(devKey.bus));
        g_ds->Print(":");
        g_ds->Print(to_hstridng(devKey.device));
        g_ds->Print(":");
        g_ds->Println(to_hstridng(devKey.function));
        return true;
    }
    return false;
}

BlockDevice* GenericIDE::CreateDevice() {

}