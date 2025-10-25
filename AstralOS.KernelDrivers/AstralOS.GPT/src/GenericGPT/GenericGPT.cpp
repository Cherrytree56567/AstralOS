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

void GenericGPTDevice::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
    BaseDriver* bsdrv = (BaseDriver*)dev;
    bldev = (BlockDevice*)bsdrv;

    uint64_t buf_phys = (uint64_t)_ds->RequestPage();
    uint64_t buf_virt = 0xFFFFFFFF00000000 + buf_phys;

    _ds->MapMemory((void*)buf_virt, (void*)buf_phys, false);

    PMBR* pmb = (PMBR*)buf_virt;

    /*
     * Make sure we zero out the buffer
    */
    memset((void*)buf_virt, 0, bldev->SectorSize());

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
    CurrentPartition = 0;

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
                PartitionCount++;
                //                                           0xASTRALOS
                PartitionMountID[PartitionCount] = (uint64_t)0xA574A105;
            }
        }
    }
}

bool GenericGPTDevice::ReadSector(uint64_t lba, void* buffer) {
    uint64_t start = Partitions[CurrentPartition].StartingLBA;
    uint64_t end = Partitions[CurrentPartition].EndingLBA;

    if ((start + lba) > ((end - start) + 1)) {
        return false;
    }

    return bldev->ReadSector(start + lba, buffer);
}

bool GenericGPTDevice::WriteSector(uint64_t lba, void* buffer) {
    uint64_t start = Partitions[CurrentPartition].StartingLBA;
    uint64_t end = Partitions[CurrentPartition].EndingLBA;

    if (lba > (end - start)) {
        return false;
    }

    return bldev->WriteSector(start + lba, buffer);
}

bool GenericGPTDevice::SetPartition(uint8_t partition) {
    if (partition > PartitionCount) {
        return false;
    }
    CurrentPartition = partition;
    return true;
}

uint64_t GenericGPTDevice::SectorCount() const {
    if (Partitions[CurrentPartition].EndingLBA >= Partitions[CurrentPartition].StartingLBA) {
        return (Partitions[CurrentPartition].EndingLBA - Partitions[CurrentPartition].StartingLBA) + 1;
    } else {
        return 0x0;
    }
}

uint32_t GenericGPTDevice::SectorSize() const {
    return bldev->SectorSize();
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

bool GenericGPTDevice::SetMount(uint64_t FSID) {
    if (PartitionMountID[CurrentPartition] != 0xA574A105) {
        return false;
    }

    PartitionMountID[CurrentPartition] = FSID;
    return true;
}

bool GenericGPTDevice::SetMountNode(FsNode* Node) {
    PartitionFsNode[CurrentPartition] = Node;
    return true;
}

const char* GenericGPTDevice::name() const {
    char cname[37];
    for (int j = 0; j < 36; j++) {
    cname[j] = (char)Partitions[PartitionCount].Name[j];
        if (Partitions[PartitionCount].Name[j] == 0) break;
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
const char* GenericGPTDevice::DriverName() const {
    return _ds->strdup("Generic GPT Driver");
}

BaseDriver* GenericGPTDevice::GetParentLayer() {
    return bldev;
}