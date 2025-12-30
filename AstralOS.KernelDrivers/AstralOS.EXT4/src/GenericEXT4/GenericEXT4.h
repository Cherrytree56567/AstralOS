#pragma once
#define DRIVER
#define FILE
#include "../PCI.h"
#include "../DriverServices.h"
#include <cstdint>
#include "../crc32c.h"

#include "Superblock/Superblock.h"
#include "BlockGroup/BlockGroup.h"
#include "Inode/Inode.h"
#include "Directory/Directory.h"
#include "Extent/Extent.h"

const char* to_hstridng(uint64_t value);
static bool isPower(uint32_t n, uint32_t base);
int memcmp(const void* a, const void* b, size_t n);
constexpr uint64_t ceil(uint64_t a, uint64_t b);
int strcmp(const char* a, const char* b);
void* memset(void* dest, uint8_t value, size_t num);
void* memcpy(void* dest, const void* src, size_t n);

#define SUPPORTED_RO_COMPAT ( \
    ROFeatures::RO_COMPAT_EXTRA_ISIZE | \
    ROFeatures::RO_COMPAT_SPARSE_SUPER | \
    ROFeatures::RO_COMPAT_DIR_NLINK | \
    ROFeatures::RO_COMPAT_HUGE_FILE | \
    ROFeatures::RO_COMPAT_LARGE_FILE | \
    ROFeatures::RO_COMPAT_METADATA_CSUM \
)

#define SUPPORTED_COMPAT ( \
    CompatibleFeatures::COMPAT_HAS_JOURNAL \
)

class GenericEXT4 : public FilesystemDriverFactory {
public:
    virtual ~GenericEXT4() {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual FilesystemDevice* CreateDevice() override;
};

class GenericEXT4Device : public FilesystemDevice {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override;
    virtual bool Support() override;
    virtual FsNode* Mount() override;
    virtual bool Unmount() override;

    virtual FsNode** ListDir(FsNode* node, size_t* outCount) override;
    virtual FsNode* FindDir(FsNode* node, const char* name) override;
    virtual FsNode* CreateDir(FsNode* parent, const char* name) override;
    virtual bool Remove(FsNode* node) override;

    virtual File* Open(const char* name, uint32_t flags) override;
    virtual bool Close(File* file) override;
    virtual int64_t Read(File* file, void* buffer, uint64_t size) override;
    virtual int64_t Write(File* file, void* buffer, uint64_t size) override;

    virtual bool Stat(FsNode* node, FsNode* out) override;
    virtual bool Chmod(FsNode* node, uint32_t mode) override;
    virtual bool Chown(FsNode* node, uint32_t uid, uint32_t gid) override;
    virtual bool Utimes(FsNode* node, uint64_t atime, uint64_t mtime, uint64_t ctime) override;

    virtual uint8_t GetClass() override;
    virtual uint8_t GetSubClass() override;
    virtual uint8_t GetProgIF() override;
    virtual const char* name() const override;
    virtual const char* DriverName() const override;
    virtual PartitionDevice* GetParentLayer() override;
private:
    Inode* ReadInode(uint64_t node);
    void WriteInode(uint32_t inodeNum, Inode* ind);
    uint32_t AllocateInode(FsNode* parent);
    uint32_t AllocateBlock(FsNode* parent);
    BlockGroupDescriptor* ReadGroupDesc(uint32_t group);
    bool HasSuperblockBKP(uint32_t group);
    void UpdateSuperblock();
    void UpdateGroupDesc(uint32_t group, BlockGroupDescriptor* GroupDesc);
    void UpdateBlockBitmapChksum(uint32_t group, BlockGroupDescriptor* GroupDesc);
    void UpdateInodeBitmapChksum(uint32_t group, BlockGroupDescriptor* GroupDesc);
    uint8_t* ReadBitmapBlock(BlockGroupDescriptor* GroupDesc);
    void WriteBitmapBlock(BlockGroupDescriptor* GroupDesc, uint8_t* bitmap);
    uint8_t* ReadBitmapInode(BlockGroupDescriptor* GroupDesc);
    void WriteBitmapInode(BlockGroupDescriptor* GroupDesc, uint8_t* bitmap);
    uint64_t CountExtents(ExtentHeader* hdr);
    Extent** GetExtents(Inode* ind, ExtentHeader* hdr, uint64_t& extentsCount);
    Extent** GetExtentsRecursive(Inode* ind, ExtentHeader* exthdr, uint64_t& extentsCount);
    FsNode** ParseDirectoryBlock(uint8_t* block, uint64_t& count);
    uint64_t ResolveExtentBlockFromHdr(ExtentHeader* hdr, uint64_t logicalBlock);
    uint64_t ResolveExtentBlock(Inode* inode, uint64_t logicalBlock);

	PartitionDevice* pdev;
	DriverServices* _ds = nullptr;
    DeviceKey devKey;

    EXT4_Superblock* superblock;
    BlockGroupDescriptor** GroupDescs;
    FsNode rootNode;
    bool isMounted;
    bool readOnly;
};