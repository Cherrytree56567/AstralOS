#define DRIVER
#include <new>
#include "GenericSATA_AHCI/GenericSATA_AHCI.h"

extern "C"
DriverInfo DriverMain(DriverServices DServices) {
    DServices.Println("Driver Loaded!");

    DriverInfo di((char*)"", 1, 0, 0);
    di.name = DServices.strdup("SATA ACHI Driver");

    void* mem = DServices.malloc(sizeof(GenericSATA_AHCI));
    if (!mem) {
        DServices.Println("Failed to Malloc for Generic SATA AHCI");
        di.exCode = 1;
        return di;
    }

    BlockDeviceFactory* factory = new(mem) GenericSATA_AHCI();
    factory->ds = &DServices;

    DServices.RegisterDriver(factory);

    DServices.Println("Reg D");

    return di;
}