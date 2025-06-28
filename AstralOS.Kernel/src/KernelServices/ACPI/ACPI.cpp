#include "ACPI.h"
#include "../KernelServices.h"

void ACPI::Initialize(BasicConsole* bc, void* rsdpAddr) {
    xsdp = (XSDP*)rsdpAddr;
    if (xsdp->Revision == 0) {
        isRSDP = true;
        rsdp = (RSDP*)rsdpAddr;
        xsdp = nullptr;
    } else {
        isRSDP = false;
        rsdp = (RSDP*)rsdpAddr;
    }

    if (isRSDP) {
        basicConsole = bc;
        basicConsole->Println("WARNING: THERE IS CURRENTLY NO SUPPORT FOR RSDP IN ASTRALOS.");
    } else {
        ks->pageTableManager.MapMemory((void*)xsdp->XsdtAddress, (void*)xsdp->XsdtAddress);
        xsdt = (XSDT*)xsdp->XsdtAddress;
        basicConsole->Print("ACPI XSDT Addr: ");
        basicConsole->Println(to_hstring((uint64_t)xsdp->XsdtAddress)); // FAILS
    }
}

bool ACPI::doChecksum(ACPISDTHeader *tableHeader) {
    unsigned char sum = 0;

    for (int i = 0; i < tableHeader->Length; i++) {
        sum += ((char *) tableHeader)[i];
    }

    return sum == 0;
}