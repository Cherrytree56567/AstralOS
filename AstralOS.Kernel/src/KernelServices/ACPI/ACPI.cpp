#include "ACPI.h"
#include "../KernelServices.h"

void ACPI::Initialize(BasicConsole* bc, void* rsdpAddr) {
    basicConsole = bc;
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
        basicConsole->Println(to_hstring(xsdp->XsdtAddress));
        if (!doChecksum(&xsdt->h)) {
            basicConsole->Println("XSDT Checksum failed!");
            return;
        }
    }
}

bool ACPI::doChecksum(ACPISDTHeader *tableHeader) {
    unsigned char sum = 0;

    for (int i = 0; i < tableHeader->Length; i++) {
        sum += ((char *) tableHeader)[i];
    }

    return sum == 0;
}

void* ACPI::GetFACP() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->PointerToOtherSDT[i];
        if (!strncmp(h->Signature, "FACP", 4)) {
            return (void *) h;
        }
    }

    basicConsole->Println("FACP not found in XSDT!");
    return NULL;
}