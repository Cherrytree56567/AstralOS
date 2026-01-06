#pragma once
#include <cstdint>
#include <cstddef>
#ifdef DRIVER
#include "PCI.h"
#include "File.h"
#else 
#include "../PCI/PCI.h"
#include "../File/File.h"
#endif

struct DriverServices;

namespace DriverType {
    enum _DriverType {
        BaseDevice,
        BlockDevice,
        BlockController,
        PartitionDriver,
        FilesystemDriver
    };
}

enum LayerType {
    OTHER = 0,
    PCIE = 1,
    SOFTWARE = 2
};

enum MountFSID {
    EXT4MountID = 0xE474
};

class BaseDriver {
public:
    virtual ~BaseDriver() {}
    virtual const char* DriverName() const = 0;
    virtual void Init(DriverServices& ds, DeviceKey& devKey) = 0;
    virtual uint8_t GetClass() = 0;
    virtual uint8_t GetSubClass() = 0;
    virtual uint8_t GetProgIF() = 0;
    virtual DriverType::_DriverType GetDriverType() = 0;
    virtual BaseDriver* GetParentLayer() { return NULL; }
};

class BaseDriverFactory {
public:
    virtual ~BaseDriverFactory() {}
    virtual bool Supports(const DeviceKey& devKey) = 0;
    virtual BaseDriver* CreateDevice() = 0;
    virtual LayerType GetLayerType() { return PCIE; }
};

class BlockDevice : public BaseDriver {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override = 0;
    virtual bool ReadSector(uint64_t lba, void* buffer) = 0;
    virtual bool WriteSector(uint64_t lba, void* buffer) = 0;

    virtual uint64_t SectorCount() const = 0;
    virtual uint32_t SectorSize() const = 0;
    virtual void* GetInternalBuffer() = 0;
    virtual const char* name() const = 0;

    virtual uint8_t GetClass() override = 0;
    virtual uint8_t GetSubClass() override = 0;
    virtual uint8_t GetProgIF() override = 0;
    virtual uint8_t GetDrive() = 0;
    virtual const char* DriverName() const override = 0;
    virtual DriverType::_DriverType GetDriverType() override {
        return DriverType::BlockDevice;
    }
    virtual BlockController* GetParentLayer() = 0;
};

class BlockDeviceFactory : public BaseDriverFactory {
public:
    virtual ~BlockDeviceFactory() {}
    virtual bool Supports(const DeviceKey& devKey) override = 0;
    virtual BlockDevice* CreateDevice() override = 0;
    virtual LayerType GetLayerType() override { return PCIE; }
};

class BlockController : public BaseDriver {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override = 0;
    virtual bool ReadSector(uint8_t drive, uint64_t lba, void* buffer) = 0;
    virtual bool WriteSector(uint8_t drive, uint64_t lba, void* buffer) = 0;

    virtual uint64_t SectorCount(uint8_t drive) const = 0;
    virtual uint32_t SectorSize(uint8_t drive) const = 0;
    virtual void* GetInternalBuffer(uint8_t drive) = 0;
    virtual const char* name(uint8_t drive) const = 0;

    virtual uint8_t GetClass() override = 0;
    virtual uint8_t GetSubClass() override = 0;
    virtual uint8_t GetProgIF() override = 0;
    virtual const char* DriverName() const override = 0;
    virtual DriverType::_DriverType GetDriverType() override {
        return DriverType::BlockController;
    }
};

class BlockControllerFactory : public BaseDriverFactory {
public:
    virtual ~BlockControllerFactory() {}
    virtual bool Supports(const DeviceKey& devKey) override = 0;
    virtual BlockController* CreateDevice() override = 0;
    virtual LayerType GetLayerType() override { return PCIE; }
};

class PartitionDevice : public BaseDriver {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override = 0;
    virtual bool ReadSector(uint64_t lba, void* buffer) = 0;
    virtual bool WriteSector(uint64_t lba, void* buffer) = 0;
    virtual bool SetPartition(uint8_t partition) = 0;

    virtual uint64_t SectorCount() const = 0;
    virtual uint32_t SectorSize() const = 0;

    virtual uint8_t GetClass() override = 0;
    virtual uint8_t GetSubClass() override = 0;
    virtual uint8_t GetProgIF() override = 0;
    virtual const char* name() const = 0;
    virtual const char* DriverName() const override = 0;
    virtual BaseDriver* GetParentLayer() override = 0;
    virtual DriverType::_DriverType GetDriverType() override {
        return DriverType::PartitionDriver;
    }
    virtual bool SetMount(uint64_t FSID) = 0;
    virtual bool SetMountNode(FsNode* Node) = 0;
    virtual uint64_t GetMount() = 0;
    virtual FsNode* GetMountNode() = 0;
};

class PartitionDriverFactory : public BaseDriverFactory {
public:
    virtual ~PartitionDriverFactory() {}
    virtual bool Supports(const DeviceKey& devKey) = 0;
    virtual PartitionDevice* CreateDevice() = 0;
    virtual LayerType GetLayerType() override { return SOFTWARE; }
};

class FilesystemDevice : public BaseDriver {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override = 0;
    virtual bool Support() = 0;
    virtual FsNode* Mount() = 0;
    virtual bool Unmount() = 0;

    virtual FsNode** ListDir(FsNode* node, size_t* outCount) = 0;
    virtual FsNode* FindDir(FsNode* node, const char* name) = 0;
    virtual FsNode* CreateDir(FsNode* parent, const char* name) = 0;
    virtual bool Remove(FsNode* node) = 0;

    virtual File* Open(const char* name, uint32_t flags) = 0;
    virtual bool Close(File* file) = 0;
    virtual int64_t Read(File* file, void* buffer, uint64_t size) = 0;
    virtual int64_t Write(File* file, void* buffer, uint64_t size) = 0;

    virtual bool Stat(FsNode* node, FsNode* out) = 0;
    virtual bool Chmod(FsNode* node, uint32_t mode) = 0;
    virtual bool Chown(FsNode* node, uint32_t uid, uint32_t gid) = 0;
    virtual bool Utimes(FsNode* node, uint64_t atime, uint64_t mtime, uint64_t ctime) = 0;

    virtual uint8_t GetClass() override = 0;
    virtual uint8_t GetSubClass() override = 0;
    virtual uint8_t GetProgIF() override = 0;
    virtual const char* name() const = 0;
    virtual const char* DriverName() const override = 0;
    virtual PartitionDevice* GetParentLayer() override = 0;
    virtual DriverType::_DriverType GetDriverType() override {
        return DriverType::FilesystemDriver;
    }
};

class FilesystemDriverFactory : public BaseDriverFactory {
public:
    virtual ~FilesystemDriverFactory() {}
    virtual bool Supports(const DeviceKey& devKey) = 0;
    virtual FilesystemDevice* CreateDevice() = 0;
    virtual LayerType GetLayerType() override { return SOFTWARE; }
};

struct DriverServices {
    /*
     * Debugging
    */
    void (*Print)(const char* str);
    void (*Println)(const char* str);

    /*
     * Memory
    */
    void* (*RequestPage)();
    void (*LockPage)(void* address);
    void (*LockPages)(void* address, uint64_t pageCount);
    void (*FreePage)(void* address);
    void (*FreePages)(void* address, uint64_t pageCount);
    void (*MapMemory)(void* virtualMemory, void* physicalMemory, bool cache);
    void (*UnMapMemory)(void* virtualMemory);
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    char* (*strdup)(const char* str);

    /*
     * IRQs
    */
    void (*SetDescriptor)(uint8_t vector, void* isr, uint8_t flags);

    /*
     * PCI/e
    */
    bool (*PCIExists)();
    bool (*PCIeExists)();
    bool (*EnableMSI)(uint8_t bus, uint8_t device, uint8_t function, uint8_t vector);
    bool (*EnableMSIx)(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector);
    uint16_t (*ConfigReadWord)(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    void (*ConfigWriteWord)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
    void (*ConfigWriteDWord)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
    uint8_t (*ConfigReadByte)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t (*ConfigReadDWord)(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint16_t (*ConfigReadWorde)(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    void (*ConfigWriteWorde)(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
    void (*ConfigWriteDWorde)(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
    uint8_t (*ConfigReadBytee)(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t (*ConfigReadDWorde)(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    /*
     * Driver Stuff
    */
    void (*RegisterDriver)(BaseDriverFactory* factory);

    /*
     * Timer Stuff
    */
    void (*sleep)(uint64_t ms);
};

struct DriverInfo {
    char* name; // Driver Name (for debugging and stuff)
    int verMaj; // Version Major
    int verMin; // Version Minor
    int exCode; // Exit Code

    //            Name     Ver Maj    Ver Min   Exit Code
    DriverInfo(char* nam, int verMa, int verMi, int Ecode) 
             : name(name), verMaj(verMa), verMin(verMi), exCode(Ecode) {}
};