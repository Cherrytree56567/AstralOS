#include "DriverServices.h"

extern "C"
DriverInfo DriverMain(DriverServices DServices) {
    DServices.Println("Driver Loaded!");

    DriverInfo di((char*)"", 1, 0, 0);
    di.name = DServices.strdup("SATA ACHI Driver");
    return di;
}