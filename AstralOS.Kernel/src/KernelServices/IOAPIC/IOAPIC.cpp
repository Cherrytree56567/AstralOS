#include "IOAPIC.h"

uint32_t IOAPIC::read(unsigned char regOff) {
    *(uint32_t volatile*) virtAddr = regOff;
    return *(uint32_t volatile*)(virtAddr + 0x10);
}
void IOAPIC::write(unsigned char regOff, uint32_t data) {
    *(uint32_t volatile*) virtAddr = regOff;
    *(uint32_t volatile*)(virtAddr + 0x10) = data;
}

/*
 * Some of this code is from here:
 * https://github.com/pdoane/osdev/blob/master/intr/ioapic.c
*/
void IOAPIC::Initialize(BasicConsole* bc, unsigned long virtAddr, unsigned long apicId, unsigned long gsib) {
    basicConsole = bc;
    this->virtAddr = virtAddr;

    this->apicId = (read(IOAPICID) >> 24) & 0xF0;
    this->apicVer = read(IOAPICVER);

    redirEntryCnt = (read(IOAPICVER) >> 16) + 1;
    globalIntrBase = gsib;

    for (uint8_t i = 0; i < redirEntryCnt; ++i) {
        RedirectionEntry entry = {};
        entry.vector = 0x20 + i; // 0x20 = typical starting interrupt vector
        entry.delvMode = 0;      // fixed delivery
        entry.destMode = 0;      // physical
        entry.mask = 1;          // start masked
        entry.destination = 0;   // LAPIC ID 0 (usually BSP)

        writeRedirEntry(i, &entry);
    }

    basicConsole->Println("IOAPIC initialized, all IRQs masked.");
}

RedirectionEntry IOAPIC::getRedirEntry(unsigned char entNo) {
    RedirectionEntry ent = {};

    if (entNo >= redirectionEntries()) {
        basicConsole->Println("Invalid redir entry number %d", entNo);
        return ent;
    }

    uint8_t lowReg = 0x10 + (entNo * 2);
    uint8_t highReg = lowReg + 1;

    ent.lowerDword = read(lowReg);
    ent.upperDword = read(highReg);

    return ent;
}

void IOAPIC::writeRedirEntry(unsigned char entNo, RedirectionEntry* entry) {
    if (entNo >= redirectionEntries()) {
        basicConsole->Println("Invalid redir entry number %d", entNo);
        return;
    }

    uint8_t lowReg = 0x10 + (entNo * 2);
    uint8_t highReg = lowReg + 1;

    write(lowReg, entry->lowerDword);
    write(highReg, entry->upperDword);
}