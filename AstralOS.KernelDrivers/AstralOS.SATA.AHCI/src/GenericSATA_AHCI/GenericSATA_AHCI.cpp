#include "GenericSATA_AHCI.h"
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

bool GenericSATA_AHCI::Supports(const DeviceKey& devKey) {
    if (devKey.classCode == 0x01) {
        uint8_t capPtr = g_ds->ConfigReadByte(devKey.bus, devKey.device, devKey.function, 0x34);
        g_ds->Print("Cap Ptr: ");
        g_ds->Print(to_hstridng((uint64_t)capPtr));
        g_ds->Print(", ");

        while (capPtr != 0) {
            uint8_t capID = g_ds->ConfigReadByte(devKey.bus, devKey.device, devKey.function, capPtr);
            if (capID == 0x01) {
                g_ds->Println("Generic SATA AHCI Device Found!");
                return true;
            }
            capPtr = g_ds->ConfigReadByte(devKey.bus, devKey.device, devKey.function, capPtr + 1);
        }
    }
    return false;
}

BlockDevice* GenericSATA_AHCI::CreateDevice() {

}