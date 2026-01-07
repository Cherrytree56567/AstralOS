#include "GenericGPTController.h"
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

bool GenericGPTControllerFactory::Supports(const DeviceKey& devKey) {
    if (devKey.bars[2] == 22) {
        uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
        BaseDriver* bsdrv = (BaseDriver*)dev;
        BlockDevice* bldev = (BlockDevice*)bsdrv;
        if (bldev) {
            if (bldev->GetDriverType() == DriverType::BlockDevice) {
                return true;
            }
        }
    }
    return false;
}

PartitionController* GenericGPTControllerFactory::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericGPTController));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic GPT Device");
        return nullptr;
    }

    GenericGPTController* device = new(mem) GenericGPTController();
    return device;
}

void* memset(void* dest, uint8_t value, size_t num) {
    uint8_t* ptr = (uint8_t*)dest;
    for (size_t i = 0; i < num; i++) {
        ptr[i] = value;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];

    return dest;
}

void GenericGPTController::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
    BaseDriver* bsdrv = (BaseDriver*)dev;
    bldev = (BlockDevice*)bsdrv;

    void* mem = _ds->malloc(sizeof(GenericGPTDeviceFactory));
    if (!mem) {
        _ds->Println("Failed to Malloc for Generic GPT Device Factory");
        _ds->Println(to_hstridng((uint64_t)mem));
    }

    /*
     * Don't Register the factory bc we don't want the DriverManager
     * to make more Generic GPT Devices, since we are handling that.
    */
    GenericGPTDeviceFactory* factory = new(mem) GenericGPTDeviceFactory();

    uint64_t buf_phys = (uint64_t)_ds->RequestPage();
    uint64_t buf_virt = 0xFFFFFFFF00000000 + buf_phys;

    _ds->MapMemory((void*)buf_virt, (void*)buf_phys, false);

    /*
     * Make sure we zero out the buffer
    */
    memset((void*)buf_virt, 0, bldev->SectorSize());

    PMBR* pmb = (PMBR*)buf_virt;

    /*
     * Usually the PMBR is in LBA0
    */
    if (bldev->ReadSector(0, (void*)buf_phys)) {
        pmbr = *pmb;
    }

    PartitionTableHeader* ahdr = (PartitionTableHeader*)buf_virt;

    memset((void*)buf_virt, 0, bldev->SectorSize());

    /*
     * Usually the Partition Table Header is in LBA1
    */
    if (bldev->ReadSector(1, (void*)buf_phys)) {
        hdr = *ahdr;
    }

    PartitionCount = 0;
    Partitions = (PartitionEntry*)_ds->malloc(hdr.numEntries * sizeof(PartitionEntry));

    uint64_t tableSectors = (hdr.numEntries * sizeof(PartitionEntry) + bldev->SectorSize() - 1) / bldev->SectorSize();
    for (uint64_t s = 0; s < tableSectors; s++) {
        uint64_t lba = hdr.lba + s;
        PartitionEntry* ape = (PartitionEntry*)buf_virt;

        memset((void*)buf_virt, 0, bldev->SectorSize());

        /*
        * Usually the Partition Table Header is in LBA2
        */
        if (bldev->ReadSector(lba, (void*)buf_phys)) {
            uint64_t entriesInSector = bldev->SectorSize() / sizeof(PartitionEntry);
            for (uint64_t i = 0; i < entriesInSector; i++) {
                uint64_t index = s * entriesInSector + i;
                if (index >= hdr.numEntries) {
                    break;
                }

                PartitionEntry* aape = &ape[i];
                if (aape->PartitionGUID[0] == 0 && aape->PartitionGUID[1] == 0) {
                    continue;
                }

                if (aape->PartitionType[0] == 0 && aape->PartitionType[1] == 0) {
                    continue;
                }

                if (aape->PartitionGUID[0] < 0x100000000000ULL || aape->PartitionGUID[1] < 0x100000000000ULL) {
                    continue;
                }
                
                if (PartitionCount >= 128) {
                    continue;
                }

                Partitions[PartitionCount] = *aape;
                //                               Unmounted - 0xASTRALOS
                PartitionMountID[PartitionCount] = (uint64_t)0xA574A105;

                BaseDriver* device = factory->CreateDevice();

                BaseDriver* dev = this;
                DeviceKey DevK;
                DevK.bars[0] = ((uint64_t)dev >> 32);
                DevK.bars[1] = ((uint64_t)dev & 0xFFFFFFFF);
                DevK.bars[2] = 2;
                DevK.bars[3] = PartitionCount;

                device->Init(ds, DevK);
                _ds->AddDriver(device);

                PartitionCount++;
            }
        }
    }
}

bool GenericGPTController::ReadSector(uint8_t partition, uint64_t lba, void* buffer) {
    uint64_t start = Partitions[partition].StartingLBA;
    uint64_t end = Partitions[partition].EndingLBA;

    if ((start + lba) > ((end - start) + 1)) {
        return false;
    }

    return bldev->ReadSector(start + lba, buffer);
}

bool GenericGPTController::WriteSector(uint8_t partition, uint64_t lba, void* buffer) {
    uint64_t start = Partitions[partition].StartingLBA;
    uint64_t end = Partitions[partition].EndingLBA;

    if (lba > (end - start)) {
        return false;
    }

    return bldev->WriteSector(start + lba, buffer);
}

uint64_t GenericGPTController::SectorCount(uint8_t partition) const {
    if (Partitions[partition].EndingLBA >= Partitions[partition].StartingLBA) {
        return (Partitions[partition].EndingLBA - Partitions[partition].StartingLBA) + 1;
    } else {
        return 0x0;
    }
}

uint32_t GenericGPTController::SectorSize(uint8_t partition) const {
    return bldev->SectorSize();
}

uint8_t GenericGPTController::GetClass() {
    return 0x0;
}

uint8_t GenericGPTController::GetSubClass() {
    return 0x0;
}

uint8_t GenericGPTController::GetProgIF() {
    return 0x0;
}

bool GenericGPTController::SetMount(uint8_t partition, uint64_t FSID) {
    PartitionMountID[partition] = FSID;
    return true;
}

bool GenericGPTController::SetMountNode(uint8_t partition, FsNode* Node) {
    PartitionFsNode[partition] = Node;
    return true;
}

uint64_t GenericGPTController::GetMount(uint8_t partition) {
    return PartitionMountID[partition];
}

FsNode* GenericGPTController::GetMountNode(uint8_t partition) {
    return PartitionFsNode[partition];
}

const char* GenericGPTController::name(uint8_t partition) const {
    char cname[37];
    for (int j = 0; j < 36; j++) {
        cname[j] = (char)Partitions[partition].Name[j];
        if (Partitions[partition].Name[j] == 0) break;
    }
    cname[36] = '\0';

    return _ds->strdup(cname);
}

/*
 * Make sure you return somthing
 * otherwise you will get weird
 * errors, like code that ends up
 * at 0x8 in mem.
*/
const char* GenericGPTController::DriverName() const {
    return _ds->strdup("Generic GPT Controller");
}

BaseDriver* GenericGPTController::GetParentLayer() {
    return bldev;
}