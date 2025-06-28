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

void* ACPI::GetMADT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->PointerToOtherSDT[i];
        if (!strncmp(h->Signature, "APIC", 4)) {
            return (void *) h;
        }
    }

    basicConsole->Println("MADT not found in XSDT!");
    return NULL;
}

void* ACPI::GetBGRT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->PointerToOtherSDT[i];
        if (!strncmp(h->Signature, "BGRT", 4)) {
            return (void *) h;
        }
    }

    basicConsole->Println("BGRT not found in XSDT!");
    return NULL;
}

/*
 * TODO: What is the Signature for DSDT?
*/
void* ACPI::GetDSDT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->PointerToOtherSDT[i];
        if (!strncmp(h->Signature, "DSDT", 4)) {
            return (void *) h;
        }
    }

    basicConsole->Println("DSDT not found in XSDT!");
    return NULL;
}

void* ACPI::GetRSDT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->PointerToOtherSDT[i];
        if (!strncmp(h->Signature, "RSDT", 4)) {
            return (void *) h;
        }
    }

    basicConsole->Println("RSDT not found in XSDT!");
    return NULL;
}

void* ACPI::GetSRAT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->PointerToOtherSDT[i];
        if (!strncmp(h->Signature, "SRAT", 4)) {
            return (void *) h;
        }
    }

    basicConsole->Println("SRAT not found in XSDT!");
    return NULL;
}

void* ACPI::GetSSDT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->PointerToOtherSDT[i];
        if (!strncmp(h->Signature, "SSDT", 4)) {
            return (void *) h;
        }
    }

    basicConsole->Println("SSDT not found in XSDT!");
    return NULL;
}