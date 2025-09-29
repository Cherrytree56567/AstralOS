#include "GenericGPT.h"
#include "../global.h"
#include <new>

const char* to_hstridng(uint64_t value) {
    static char buffer[19];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    do {
        uint8_t nibble = value & 0xF;
        *--ptr = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    } while (value > 0);

    *--ptr = 'x';
    *--ptr = '0';

    return ptr;
}

bool GenericGPT::Supports(const DeviceKey& devKey) {
    if (devKey.bars[2] == 22) {
        uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
        BaseDriver* bsdrv = (BaseDriver*)dev;
        BlockDevice* bldev = (BlockDevice*)bsdrv;
        if (bldev) {
            if (bldev->GetClass() == 0x1) {
                return true;
            }
        }
    }
    return false;
}

PartitionDevice* GenericGPT::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericGPTDevice));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic GPT Device");
        return nullptr;
    }

    GenericGPTDevice* device = new(mem) GenericGPTDevice();
    return device;
}

void GenericGPTDevice::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
    BaseDriver* bsdrv = (BaseDriver*)dev;
    bldev = (BlockDevice*)bsdrv;

    _ds->Println("Generic GPT Initialized!");
}

bool GenericGPTDevice::ReadSector(uint64_t lba, void* buffer) {
    return true;
}

bool GenericGPTDevice::WriteSector(uint64_t lba, void* buffer) {
    return true;
}

bool GenericGPTDevice::SetParition(uint8_t partition) {
    return true;
}

uint64_t GenericGPTDevice::SectorCount() const {
    return 0;
}

uint32_t GenericGPTDevice::SectorSize() const {
    return 0;
}

uint8_t GenericGPTDevice::GetClass() {
    return 0x0;
}

uint8_t GenericGPTDevice::GetSubClass() {
    return 0x0;
}

uint8_t GenericGPTDevice::GetProgIF() {
    return 0x0;
}

const char* GenericGPTDevice::name() const {
    return "";
}

/*
 * Make sure you return somthing
 * otherwise you will get weird
 * errors, like code that ends up
 * at 0x8 in mem.
*/
const char* GenericGPTDevice::DriverName() const {
    return "Generic GPT Driver";
}

BaseDriver* GenericGPTDevice::GetParentLayer() {
    return bldev;
}