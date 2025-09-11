#include "GenericSATA_AHCI.h"
#include "../global.h"

bool GenericSATA_AHCI::Supports(const DeviceKey& devKey) {
    ds->Print("Checking if Supports: ");
    return false;
    if (devKey.classCode == 0x01) {
        uint8_t capPtr = ds->ConfigReadByte(devKey.bus, devKey.device, devKey.function, 0x34);

        while (capPtr != 0) {
            uint8_t capID = ds->ConfigReadByte(devKey.bus, devKey.device, devKey.function, capPtr);
            if (capID == 0x01) {
                ds->Println("Generic SATA AHCI Device Found!");
                return true;
            }
            capPtr = ds->ConfigReadByte(devKey.bus, devKey.device, devKey.function, capPtr + 1);
        }
    }
    ds->Println("Generic SATA AHCI Device Not Found!");
    return false;
}

BlockDevice* GenericSATA_AHCI::CreateDevice() {

}