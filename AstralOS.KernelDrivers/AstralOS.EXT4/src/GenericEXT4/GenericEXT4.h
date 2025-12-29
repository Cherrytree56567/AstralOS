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

const char* to_hstridng(uint64_t value);
static bool isPower(uint32_t n, uint32_t base);
int memcmp(const void* a, const void* b, size_t n);
constexpr uint64_t ceil(uint64_t a, uint64_t b);
int strcmp(const char* a, const char* b);
void* memset(void* dest, uint8_t value, size_t num);
void* memcpy(void* dest, const void* src, size_t n);




















struct DirectoryEntry {
    uint32_t Inode; // Inode
    uint16_t TotalSize; // Total size of this entry (Including all subfields) 
    uint8_t NameLen; // Name Length least-significant 8 bits 
    uint8_t Type; // Type indicator (only if the feature bit for "directory entries have file type byte" is set, else this is the most-significant 8 bits of the Name Length) 
    char Name[]; // Size is NameLen, Name characters 
} __attribute__((packed));

struct DirectoryEntryTail {
    uint32_t det_reserved_zero1; // Inode number, which must be zero.
    uint16_t TotalSize; // Length of this directory entry, which must be 12.
    uint8_t Reserved; // Length of the file name, which must be zero.
    uint8_t Reserved2; // File type, which must be 0xDE.
    uint32_t Checksum; // Directory leaf block checksum.
} __attribute__((packed));

enum DirectoryEntryType {
    DETUnknown = 0,
    DETFile = 1,
    DETDirectory = 2,
    DETCharacterDevice = 3,
    DETBlockDevice = 4,
    DETFIFO = 5,
    DETSocket = 6,
    DETSymLink = 7
};

struct MMP {
    uint32_t Signature; // MMP signature (0x004d4d50) 
    uint32_t SequenceNum; // Sequence Number
    uint64_t LastUpdated; // Last updated time (does not affect algorithm)
    char Hostname[64]; // Hostname of system that open() the filesystem (does not affect algorithm)
    char MountPath[32]; // Mount path of system that open() the filesystem (does not affect algorithm)
    uint16_t Interval; // Interval to check MMP block 
    uint16_t Padding; // Padding. 
    uint8_t Padding1[904]; // Padding. 
    uint32_t Checksum; // Checksum (crc32c(UUID+MMP Block number)) 
} __attribute__((packed));

struct ExtentHeader {
    uint16_t magic; // 0xF30A
    uint16_t entries;
    uint16_t max;
    uint16_t depth;
    uint32_t generation;
} __attribute__((packed));

struct Extent {
    uint32_t block;
    uint16_t len;
    uint16_t startHigh;
    uint32_t startLow;
} __attribute__((packed));

struct ExtentIDX {
    uint32_t Block;
    uint32_t LeafLow;
    uint16_t LeafHigh;
    uint16_t Unused;
} __attribute__((packed));

struct Journal {
    uint32_t Signature; // Magic signature (0xc03b3998) 
    uint32_t BlockType; // Block Type
    uint32_t Transaction; // Journal transaction for this block.
} __attribute__((packed));

struct JournalSuperBlock {
    Journal header; // Journal header
    uint32_t BlockSize; // Block size of the journal device. 
    uint32_t TotalBlocks; // Total number of blocks in the journal device. 
    uint32_t FirstBlockInfo; // First block of journal information. 
    uint32_t FirstTransaction; // First journal transaction expected. 
    uint32_t FirstBlock; // First block of the journal. 
    uint32_t Error; // Errno, if the journal has an error. 
    uint32_t RequiredFeatures; // Required features present. 
    uint32_t OptionalFeatures; // Optional features present. 
    uint32_t ROFeatures; // Features that if not supported the journal must be mounted read-only. There are no read-only features as of Linux 5.9rc3. 
    uint64_t UUID[2]; // Journal UUID. 
    uint32_t FSnum; // Number of filesystems using this journal. 
    uint32_t BlockNumCopy; // Block number of the journal superblock copy. 
    uint32_t MaxJBlocks; // Maximum journal blocks per transaction. This is unused as of Linux 5.9rc3. 
    uint32_t MaxDBlocks; // Maximum data blocks per transaction. This is unused as of Linux 5.9rc3. 
    uint8_t ChecksumAlgo; // Checksum algorithm. 
    uint8_t Padding[3]; // Padding. 
    uint8_t Padding1[168]; // Padding. 
    uint32_t Checksum; // Checksum of the journal superblock. 
    uint8_t Users[16 * 48]; // From Kernel.ORG. ids of all file systems sharing the log. e2fsprogs/Linux don’t allow shared external journals, but I imagine Lustre (or ocfs2?), which use the jbd2 code, might.
} __attribute__((packed));

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