#include "GenericGPT.h"
#include "../global.h"

bool GenericGPTDeviceFactory::Supports(const DeviceKey& devKey) {
    if (devKey.bars[2] == 22) {
        uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
        BaseDriver* bsdrv = (BaseDriver*)dev;
        PartitionController* bldev = (PartitionController*)bsdrv;
        if (bldev) {
            if (bldev->GetDriverType() == DriverType::PartitionController) {
                return true;
            }
        }
    }
    return false;
}

PartitionDevice* GenericGPTDeviceFactory::CreateDevice() {
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
    pcdev = (PartitionController*)bsdrv;
    partition = devKey.bars[3];
}

bool GenericGPTDevice::ReadSector(uint64_t lba, void* buffer) {
    return pcdev->ReadSector(partition, lba, buffer);
}

bool GenericGPTDevice::WriteSector(uint64_t lba, void* buffer) {
    return pcdev->WriteSector(partition, lba, buffer);
}

uint64_t GenericGPTDevice::SectorCount() const {
    return pcdev->SectorCount(partition);
}

uint32_t GenericGPTDevice::SectorSize() const {
    return pcdev->SectorSize(partition);
}

uint8_t GenericGPTDevice::GetClass() {
    return pcdev->GetClass();
}

uint8_t GenericGPTDevice::GetSubClass() {
    return pcdev->GetSubClass();
}

uint8_t GenericGPTDevice::GetProgIF() {
    return pcdev->GetProgIF();
}

uint8_t GenericGPTDevice::GetPartition() {
    return partition;
}

bool GenericGPTDevice::SetMount(uint64_t FSID) {
    return pcdev->SetMount(partition, FSID);
}

bool GenericGPTDevice::SetMountNode(FsNode* Node) {
    return pcdev->SetMountNode(partition, Node);
}

uint64_t GenericGPTDevice::GetMount() {
    return pcdev->GetMount(partition);
}

FsNode* GenericGPTDevice::GetMountNode() {
    return pcdev->GetMountNode(partition);
}

const char* GenericGPTDevice::name() const {
    return pcdev->name(partition);
}

/*
 * Make sure you return somthing
 * otherwise you will get weird
 * errors, like code that ends up
 * at 0x8 in mem.
*/
const char* GenericGPTDevice::DriverName() const {
    return _ds->strdup("Generic GPT Device");
}

BlockDevice* GenericGPTDevice::GetParentLayer() {
    return pcdev->GetParentLayer();
}