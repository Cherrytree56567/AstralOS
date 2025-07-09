#include "ACPI.h"
#include "../KernelServices.h"

#define KERNEL_VIRT_ADDR 0xFFFFFFFF80000000

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

uint64_t ReadUnaligned64(uint8_t* addr) {
    uint64_t val;
    memcpy(&val, addr, sizeof(val));
    return val;
}

void ACPI::Initialize(BasicConsole* bc, void* rsdpAddr) {
    basicConsole = bc;

    RSDP* rsdpBase = (RSDP*)rsdpAddr;

    basicConsole->Print("RSDP Addr: ");
    basicConsole->Println(to_hstring((uint64_t)rsdpBase));
    basicConsole->Print("RSDP Signature: ");
    basicConsole->Println(rsdpBase->Signature);
    basicConsole->Print("RSDP OEM ID: ");
    basicConsole->Println(rsdpBase->OEMID);
    basicConsole->Print("RSDP Revision: ");
    basicConsole->Println(to_hstring(rsdpBase->Revision));

    if (rsdpBase->Revision == 0) {
        isRSDP = true;
        rsdp = rsdpBase;
        xsdp = nullptr;
        basicConsole->Println("WARNING: THERE IS CURRENTLY NO SUPPORT FOR RSDP IN ASTRALOS.");
    } else if (rsdpBase->Revision > 1) {
        isRSDP = false;
        xsdp = (XSDP*)rsdpAddr;
        rsdp = rsdpBase;

        basicConsole->Print("ACPI XSDT Addr: ");
        basicConsole->Println(to_hstring((KERNEL_VIRT_ADDR + xsdp->XsdtAddress)));

        ks->pageTableManager.MapMemory((void*)(KERNEL_VIRT_ADDR + xsdp->XsdtAddress), (void*)xsdp->XsdtAddress, false);
        xsdt = (XSDT*)(KERNEL_VIRT_ADDR + xsdp->XsdtAddress);

        basicConsole->Print("XSDT Length: ");
        basicConsole->Println(to_hstring(xsdt->h.Length));
        basicConsole->Print("XSDT Signature: ");
        basicConsole->Println(xsdt->h.Signature);
        basicConsole->Print("XSDT OEM ID: ");
        basicConsole->Println(xsdt->h.OEMID);

        int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
        basicConsole->Print("XSDT Entries: ");
        basicConsole->Println(to_hstring((uint64_t)entries));

        if (!doChecksum(&xsdt->h)) {
            basicConsole->Println("XSDT Checksum failed!");
            return;
        }
    } else {
        basicConsole->Println("RSDP Revision is not supported!");
        basicConsole->Println("Could be garbage values.");
        return;
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
    uint8_t* entriesBase = (uint8_t*)xsdt + sizeof(ACPISDTHeader);

    for (int i = 0; i < entries; i++) {
        uint64_t entryAddr = ReadUnaligned64(entriesBase + i * sizeof(uint64_t));

        ks->pageTableManager.MapMemory((void*)(KERNEL_VIRT_ADDR + entryAddr), (void*)entryAddr, false);

        ACPISDTHeader* h = (ACPISDTHeader*)(KERNEL_VIRT_ADDR + entryAddr);

        if (strncmp((const char*)h->Signature, "FACP", 4) == 0) {
            return (FADT*)h;
        }
    }

    basicConsole->Println("FACP not found in XSDT!");
    return NULL;
}

MADT* ACPI::GetMADT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    uint8_t* entriesBase = (uint8_t*)xsdt + sizeof(ACPISDTHeader);

    for (int i = 0; i < entries; i++) {
        uint64_t entryAddr = ReadUnaligned64(entriesBase + i * sizeof(uint64_t));

        ks->pageTableManager.MapMemory((void*)(KERNEL_VIRT_ADDR + entryAddr), (void*)entryAddr, false);

        ACPISDTHeader* h = (ACPISDTHeader*)(KERNEL_VIRT_ADDR + entryAddr);

        if (strncmp((const char*)h->Signature, "APIC", 4) == 0) {
            return (MADT*)h;
        }
    }

    basicConsole->Println("MADT not found in XSDT!");
    return nullptr;
}

BGRT* ACPI::GetBGRT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    uint8_t* entriesBase = (uint8_t*)xsdt + sizeof(ACPISDTHeader);

    for (int i = 0; i < entries; i++) {
        uint64_t entryAddr = ReadUnaligned64(entriesBase + i * sizeof(uint64_t));

        ks->pageTableManager.MapMemory((void*)(KERNEL_VIRT_ADDR + entryAddr), (void*)entryAddr, false);

        ACPISDTHeader* h = (ACPISDTHeader*)(KERNEL_VIRT_ADDR + entryAddr);

        if (strncmp((const char*)h->Signature, "BGRT", 4) == 0) {
            return (BGRT*)h;
        }
    }

    basicConsole->Println("BGRT not found in XSDT!");
    return NULL;
}

RSDT* ACPI::GetRSDT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    uint8_t* entriesBase = (uint8_t*)xsdt + sizeof(ACPISDTHeader);

    for (int i = 0; i < entries; i++) {
        uint64_t entryAddr = ReadUnaligned64(entriesBase + i * sizeof(uint64_t));

        ks->pageTableManager.MapMemory((void*)(KERNEL_VIRT_ADDR + entryAddr), (void*)entryAddr, false);

        ACPISDTHeader* h = (ACPISDTHeader*)(KERNEL_VIRT_ADDR + entryAddr);

        if (strncmp((const char*)h->Signature, "RSDT", 4) == 0) {
            return (RSDT*)h;
        }
    }

    basicConsole->Println("RSDT not found in XSDT!");
    return NULL;
}

SRAT* ACPI::GetSRAT() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    uint8_t* entriesBase = (uint8_t*)xsdt + sizeof(ACPISDTHeader);

    for (int i = 0; i < entries; i++) {
        uint64_t entryAddr = ReadUnaligned64(entriesBase + i * sizeof(uint64_t));

        ks->pageTableManager.MapMemory((void*)(KERNEL_VIRT_ADDR + entryAddr), (void*)entryAddr, false);

        ACPISDTHeader* h = (ACPISDTHeader*)(KERNEL_VIRT_ADDR + entryAddr);

        if (strncmp((const char*)h->Signature, "SRAT", 4) == 0) {
            return (SRAT*)h;
        }
    }

    basicConsole->Println("SRAT not found in XSDT!");
    return NULL;
}

MCFG* ACPI::GetMCFG() {
    int entries = (xsdt->h.Length - sizeof(xsdt->h)) / 8;
    uint8_t* entriesBase = (uint8_t*)xsdt + sizeof(ACPISDTHeader);

    for (int i = 0; i < entries; i++) {
        uint64_t entryAddr = ReadUnaligned64(entriesBase + i * sizeof(uint64_t));

        ks->pageTableManager.MapMemory((void*)(KERNEL_VIRT_ADDR + entryAddr), (void*)entryAddr, false);

        ACPISDTHeader* h = (ACPISDTHeader*)(KERNEL_VIRT_ADDR + entryAddr);

        if (strncmp((const char*)h->Signature, "MCFG", 4) == 0) {
            return (MCFG*)h;
        }
    }

    basicConsole->Println("MCFG not found in XSDT!");
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

    basicConsole->Println("Processor Local APIC not found in MADT!");
    return nullptr;
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
    basicConsole->Println("I/O APIC not found in MADT!");
    return nullptr;
}