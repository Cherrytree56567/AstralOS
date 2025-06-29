#include "ACPI.h"
#include "../KernelServices.h"

extern "C" int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) return (unsigned char)s1[i] - (unsigned char)s2[i];
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

extern "C" void* memcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    for (size_t i = 0; i < count; ++i) {
        d[i] = s[i];
    }

    return dest;
}

void ACPI::Initialize(BasicConsole* bc, void* rsdpAddr) {
    basicConsole = bc;

    // Always start by treating it as a base RSDP
    RSDP* rsdpBase = (RSDP*)rsdpAddr;

    // Print the signature safely
    char sig[9];
    memcpy(sig, rsdpBase->Signature, 8);
    sig[8] = '\0';
    basicConsole->Print("ACPI Signature: ");
    basicConsole->Println(sig);

    basicConsole->Print("ACPI RSDP Addr: ");
    basicConsole->Println(to_hstring((uint64_t)rsdpAddr));

    basicConsole->Print("ACPI Revision: ");
    basicConsole->Println(to_string((uint64_t)rsdpBase->Revision));

    if (rsdpBase->Revision == 0) {
        isRSDP = true;
        rsdp = rsdpBase;
        xsdp = nullptr;
        basicConsole->Println("WARNING: THERE IS CURRENTLY NO SUPPORT FOR RSDP IN ASTRALOS.");
    } else {
        isRSDP = false;
        xsdp = (XSDP*)rsdpAddr;
        rsdp = rsdpBase;

        // Map and validate XSDT
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

FADT* ACPI::GetFADT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->Entries[i];
        if (!strncmp(h->Signature, "FACP", 4)) {
            return (FADT*) h;
        }
    }

    basicConsole->Println("FACP not found in XSDT!");
    return NULL;
}

MADT* ACPI::GetMADT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    basicConsole->Print("MADT Entries: ");
    basicConsole->Println(to_hstring((uint64_t)xsdt->h.Length));

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->Entries[i];
        basicConsole->Print("Signature: ");
        basicConsole->Println(h->Signature);
        if (!strncmp(h->Signature, "APIC", 4)) {
            return (MADT*) h;
        }
    }

    basicConsole->Println("MADT not found in XSDT!");
    return NULL;
}

BGRT* ACPI::GetBGRT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->Entries[i];
        if (!strncmp(h->Signature, "BGRT", 4)) {
            return (BGRT*) h;
        }
    }

    basicConsole->Println("BGRT not found in XSDT!");
    return NULL;
}

RSDT* ACPI::GetRSDT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->Entries[i];
        if (!strncmp(h->Signature, "RSDT", 4)) {
            return (RSDT*) h;
        }
    }

    basicConsole->Println("RSDT not found in XSDT!");
    return NULL;
}

SRAT* ACPI::GetSRAT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;

    for (int i = 0; i < entries; i++) {
        ACPISDTHeader *h = (ACPISDTHeader *) xsdt->Entries[i];
        if (!strncmp(h->Signature, "SRAT", 4)) {
            return (SRAT*) h;
        }
    }

    basicConsole->Println("SRAT not found in XSDT!");
    return NULL;
}

MADT_ProcessorLocalAPIC* ACPI::GetProcessorLocalAPIC() {
    MADT* madt = GetMADT();
    if (!madt) {
        basicConsole->Println("MADT not found, cannot get Processor Local APIC.");
        return nullptr;
    }
    uint8_t* start = madt->Entries;
    uint8_t* end = (uint8_t*)madt + madt->header.Length;

    for (uint8_t* ptr = start; ptr < end; ) {
        MADTEntryHeader* hdr = (MADTEntryHeader*)ptr;

        if (hdr->Type == 0) {
            return (MADT_ProcessorLocalAPIC*)hdr;
        }

        ptr += hdr->Length;
    }
}

MADT_IOAPIC* ACPI::GetIOAPIC() {
    MADT* madt = GetMADT();
    if (!madt) {
        basicConsole->Println("MADT not found, cannot get I/O APIC.");
        return nullptr;
    }
    uint8_t* start = madt->Entries;
    uint8_t* end = (uint8_t*)madt + madt->header.Length;

    for (uint8_t* ptr = start; ptr < end; ) {
        MADTEntryHeader* hdr = (MADTEntryHeader*)ptr;

        if (hdr->Type == 1) {
            return (MADT_IOAPIC*)hdr;
        }

        ptr += hdr->Length;
    }
}