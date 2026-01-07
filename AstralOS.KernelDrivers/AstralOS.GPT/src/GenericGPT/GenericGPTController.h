#pragma once
#define DRIVER
#define FILE
#include <cstdint>
#include "../PCI.h"
#include "../DriverServices.h"
#include "GenericGPT.h"

/*
 * PMBR stands for Protective MBR
 * It's used, so that if an older
 * program or OS tries to read it
 * they won't corrupt the disk.
 * 
 * btw, the info was from the OSDev Wiki:
 * https://wiki.osdev.org/GPT
*/
struct PMBR {
    uint8_t BootIndicator;
    uint8_t StartingCHS[3];
    uint8_t OSType;
    uint8_t EndingCHS[3];
    uint32_t StartingLBA;
    uint32_t EndingLBA;
} __attribute__((packed));

struct PartitionTableHeader {
    uint64_t signature;
    uint32_t revision;
    uint32_t headerSize;
    uint32_t crc32check;
    uint32_t reserved;
    uint64_t lba;
    uint64_t AltLBA;
    uint64_t firstBlock;
    uint64_t lastBlock;
    uint64_t GUID[2];
    uint64_t startingLBA;
    uint32_t numEntries;
    uint32_t sizeEntries;
    uint32_t crc32;
} __attribute__((packed));

struct PartitionEntry {
    uint64_t PartitionType[2];
    uint64_t PartitionGUID[2];
    uint64_t StartingLBA;
    uint64_t EndingLBA;
    uint64_t Attributes;
    char16_t Name[36];
} __attribute__((packed));

class GenericGPTControllerFactory : public PartitionControllerFactory {
public:
    virtual ~GenericGPTControllerFactory() {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual PartitionController* CreateDevice() override;
};

class GenericGPTController : public PartitionController {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override;
    virtual bool ReadSector(uint8_t partition, uint64_t lba, void* buffer) override;
    virtual bool WriteSector(uint8_t partition, uint64_t lba, void* buffer) override;

    virtual uint64_t SectorCount(uint8_t partition) const override;
    virtual uint32_t SectorSize(uint8_t partition) const override;

    virtual uint8_t GetClass() override;
    virtual uint8_t GetSubClass() override;
    virtual uint8_t GetProgIF() override;
    virtual const char* name(uint8_t partition) const override;
    virtual const char* DriverName() const override;
    virtual BaseDriver* GetParentLayer() override;
    virtual bool SetMount(uint8_t partition, uint64_t FSID) override;
    virtual bool SetMountNode(uint8_t partition, FsNode* Node) override;
    virtual uint64_t GetMount(uint8_t partition) override;
    virtual FsNode* GetMountNode(uint8_t partition) override;
private:
	BlockDevice* bldev;
	DriverServices* _ds = nullptr;
    DeviceKey devKey;

    PMBR pmbr;
    PartitionTableHeader hdr;
    PartitionEntry* Partitions;
    uint64_t PartitionCount = 0;
    uint64_t PartitionMountID[128];
    FsNode* PartitionFsNode[128];
};