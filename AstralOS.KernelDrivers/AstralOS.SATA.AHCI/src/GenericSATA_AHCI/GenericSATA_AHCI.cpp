#include "GenericSATA_AHCI.h"
#include "../global.h"

bool GenericSATA_AHCI::Supports(const DeviceKey& devKey) {
    if (devKey.classCode == 0x1 && // Mass Storage Controller
        devKey.subclass  == 0x6 && // Serial ATA Controller
        devKey.progIF    == 0x1) { // AHCI 1.0
        ds.Println("Generic SATA AHCI Device Found!");
        return true;
    }
    return false;
}

BlockDevice* GenericSATA_AHCI::CreateDevice() {

}