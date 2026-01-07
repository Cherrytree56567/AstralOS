#include "GenericIDE.h"

bool GenericIDEFactory::Supports(const DeviceKey& devKey) {
    if (devKey.bars[2] == 22) {
        uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
        BaseDriver* bsdrv = (BaseDriver*)dev;
        BlockController* bldev = (BlockController*)bsdrv;
        if (bldev) {
            if (bldev->GetDriverType() == DriverType::BlockController) {
                return true;
            }
        }
    }
    return false;
}

BlockDevice* GenericIDEFactory::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericIDE));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic IDE Device");
        return nullptr;
    }

    GenericIDE* device = new(mem) GenericIDE();
    return device;
}

void GenericIDE::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
    BaseDriver* bsdrv = (BaseDriver*)dev;
    pdev = (BlockController*)bsdrv;
    drive = devKey.bars[3];
}

bool GenericIDE::ReadSector(uint64_t lba, void* buffer) {
    return pdev->ReadSector(drive, lba, buffer);
}

bool GenericIDE::WriteSector(uint64_t lba, void* buffer) {
    return pdev->WriteSector(drive, lba, buffer);
}

uint64_t GenericIDE::SectorCount() const {
    return pdev->SectorCount(drive);
}

uint32_t GenericIDE::SectorSize() const {
    return pdev->SectorSize(drive);
}

/*
 * Unnecessary
*/
void* GenericIDE::GetInternalBuffer() {
    return pdev->GetInternalBuffer(drive);
}

const char* GenericIDE::name() const {
    return pdev->name(drive);
}

uint8_t GenericIDE::GetClass() {
    return pdev->GetClass();
}

uint8_t GenericIDE::GetSubClass() {
    return pdev->GetSubClass();
}

uint8_t GenericIDE::GetProgIF() {
    return pdev->GetProgIF();
}

uint8_t GenericIDE::GetDrive() {
    return drive;
}

const char* GenericIDE::DriverName() const {
    return _ds->strdup("Generic IDE Device");
}

BlockController* GenericIDE::GetParentLayer() {
    return pdev;
}