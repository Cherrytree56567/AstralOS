#include "GenericAHCI.h"

void GenericAHCI::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
    BaseDriver* bsdrv = (BaseDriver*)dev;
    pdev = (BlockController*)bsdrv;
    drive = devKey.bars[2];
}

bool GenericAHCI::ReadSector(uint64_t lba, void* buffer) {
    return pdev->ReadSector(drive, lba, buffer);
}

bool GenericAHCI::WriteSector(uint64_t lba, void* buffer) {
    return pdev->WriteSector(drive, lba, buffer);
}

uint64_t GenericAHCI::SectorCount() const {
    return pdev->SectorCount(drive);
}

uint32_t GenericAHCI::SectorSize() const {
    return pdev->SectorCount(drive);
}

/*
 * Unnecessary
*/
void* GenericAHCI::GetInternalBuffer() {
    return pdev->GetInternalBuffer(drive);
}

const char* GenericAHCI::name() const {
    return pdev->name(drive);
}

uint8_t GenericAHCI::GetClass() {
    return pdev->GetClass();
}

uint8_t GenericAHCI::GetSubClass() {
    return pdev->GetSubClass();
}

uint8_t GenericAHCI::GetProgIF() {
    return pdev->GetProgIF();
}

uint8_t GenericAHCI::GetDrive() {
    return drive;
}

const char* GenericAHCI::DriverName() const {
    return _ds->strdup("GenericAHCI");
}

BlockController* GenericAHCI::GetParentLayer() {
    return pdev;
}