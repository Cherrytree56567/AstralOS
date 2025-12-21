#pragma once
#define DRIVER
#define FILE
#include "../PCI.h"
#include "../DriverServices.h"
#include <cstdint>
#include "../crc32c.h"

/*
 * This struct is inspired by the OSDev Wiki:
 * https://wiki.osdev.org/Ext4
 * 
 * Here is some basic info on the EXT4 FS if
 * you want to spin up your own.
 * 
 * The Ext2 file system divides up disk space 
 * into logical blocks of contiguous space. The 
 * size of blocks need not be the same size as 
 * the sector size of the disk the file system 
 * resides on. The size of blocks can be found 
 * by reading the Superblock.
 * 
 * A block group is just a bunch of blocks.
 * Block Groups can hold normal blocks or just
 * a bitmap of free blocks within the group or
 * a bitmap of used inodes within the group or
 * a table of inode structures that belong to 
 * the group or a backup of the EXT SuperBlock
 * 
 * An Inode represents a dir, file or symlink.
 * An Inode doesn't contain the data, but it
 * contains information about the data like 
 * permissions, name, etc.
 * 
 * The EXT Superblock contains everything abt
 * the filesystem. It is always located at byte
 * 1024 from the beginning.
 * 
 * The Root Dir's Inode is always Inode 2
*/
struct EXT4_Superblock {
    uint32_t TotalInodes; // Total number of inodes
    uint32_t TotalBlocks; // Total number of blocks
    uint32_t ReservedBlocks; // Number of reserved Blocks
    uint32_t UnallocBlocks; // Total Number of Unallocated Blocks
    uint32_t UnallocInodes; // Total Number of Unallocated Inodes
    uint32_t FirstDataBlock; // Block number of the block containing the superblock
    uint32_t BlockSize; // Block size is 1024 << BlockSize
    uint32_t FragmentSize; // Fragment size is 1024 << FragmentSize
    uint32_t BlocksPerBlockGroup; // Number of blocks in each block group 
    uint32_t FragmentsPerBlockGroup; // Number of fragments in each block group
    uint32_t InodesPerBlockGroup; // Number of inodes in each block group
    uint32_t LastMount; // Last mount time (in POSIX time)
    uint32_t LastWritten; // Last written time (in POSIX time) 
    uint16_t VolumeMountLastCheck; // Number of times the volume has been mounted since its last consistency check (fsck) 
    uint16_t MountsAllowedBeforeCheck; // Number of mounts allowed before a consistency check (fsck) must be done 
    uint16_t MagicSignature; // Magic signature (0xef53), used to help confirm the presence of Ext4 on a volume
    uint16_t FSstate; // File system state
    uint16_t ErrorPolicy; // What to do when an error is detected 
    uint16_t MinorVersion; // Minor portion of version
    uint32_t LastCheck; // POSIX time of last consistency check
    uint32_t IntervalChecks; // Interval (in POSIX time) between forced consistency checks (fsck) 
    uint32_t OSId; // Operating system ID from which the filesystem on this volume was created
    uint32_t MajorVersion; // Major portion of version (combine with Minor portion above to construct full version field) 
    uint16_t ReserveUserID; // User ID that can use reserved blocks 
    uint16_t ReserveGroupID; // Group ID that can use reserved blocks 

    /*
     * For Dynamic Superblocks Only
     *
     * If a bit is set in the required feature
     * set var it does not recognize, it must 
     * refuse to mount the filesystem. Filesystem checks, however, must abort on any unrecognized flag in the optional or required features. 
    */
    uint32_t FirstInode; // First non-reserved inode in file system.
    uint16_t InodeSize; // Size of each inode structure in bytes. 
    uint16_t BackupBlock; // Block group that this superblock is part of for backup copies. 
    uint32_t OptionalFeatures; // Optional features present. 
    uint32_t RequiredFeatures; // Required features present. 
    uint32_t ReqFeaturesRO; // Features that if not supported the volume must be mounted read-only.
    uint64_t FilesystemUUID[2]; // File system UUID. 
    char VolumeName[16]; // Volume name. 
    char LastMountPath[64]; // Path Volume was last mounted to. 
    uint32_t CompressionAlgo; // Compression algorithm used. 
    uint8_t FilePreAllocBlocks; // Amount of blocks to preallocate for files 
    uint8_t DirPreAllocBlocks; // Amount of blocks to preallocate for directories. 
    uint16_t ReservedGDTEntries; // Amount of reserved GDT entries for filesystem expansion. 
    uint64_t JournalUUID[2]; // Journal UUID.
    uint32_t JournalInode; // Journal Inode. 
    uint32_t JournalDevNum; // Journal Device number. 
    uint32_t HeadOrphanInode; // Head of orphan inode list. 
    uint32_t HTREE_Hash[4]; // HTREE hash seed in an array of 32 bit integers. 
    uint8_t HashAlgo; // Hash algorithm to use for directories. 
    uint8_t JournalBlocksField; // Journal blocks field contains a copy of the inode's block array and size. 
    uint16_t GroupDescriptorBytes; // Size of group descriptors in bytes, for 64 bit mode. 
    uint32_t MountOptions32; // Mount options. 
    uint32_t FirstMetadataBlockGroup; // First metablock block group, if enabled. 
    uint32_t FSCreationTime; // Filesystem Creation Time. 
    uint32_t JournalBackup[17]; // Journal Inode Backup in an array of 32 bit integers. 

    /*
     * 64-Bit Support
    */
    uint32_t HighBlocks; // High 32-bits of the total number of blocks. 
    uint32_t HighReservedBlocks; // High 32-bits of the total number of reserved blocks.
    uint32_t HighUnalloedBlocks; // High 32-bits of the total number of unallocated blocks.
    uint16_t MinInodeSize; // Minimum inode size. 
    uint16_t MinInodeResSize; // Minimum inode reservation size. 
    uint32_t MiscFlags; // Misc flags, such as sign of directory hash or development status. 
    uint16_t LogicalBlocksRW; // Amount logical blocks read or written per disk in a RAID array.
    uint16_t MMPCWait; // Amount of seconds to wait in Multi-mount prevention checking. 
    uint64_t MultiMountPreventBlock; // Block to multi-mount prevent. 
    uint32_t RetBlocksRW; // Amount of blocks to read or write before returning to the current disk in a RAID array. Amount of disks * stride. 
    uint8_t GroupsPerFlex; // log2 (groups per flex) - 10. (In other words, the number to shift 1,024 to the left by to obtain the groups per flex block group
    uint8_t MetaCheckAlgo; // Metadata checksum algorithm used. Linux only supports crc32. 
    uint8_t EncryptionVerLvl; // Encryption version level. 
    uint8_t Reserved; // Reserved padding. 
    uint64_t kbWritten; // Amount of kilobytes written over the filesystem's lifetime. 
    uint32_t ActiveSnapInode; // Inode number of the active snapshot. 
    uint32_t ActiveSnapSeqID; // Sequential ID of active snapshot. 
    uint64_t ActiveSnapBlocksRes; // Number of blocks reserved for active snapshot. 
    uint32_t InodeHeadDSL; // Inode number of the head of the disk snapshot list. 
    uint32_t Errors; // Amount of errors detected. 
    uint32_t FirstError; // First time an error occurred in POSIX time. 
    uint32_t FirstErrorInode; // Inode number in the first error. 
    uint32_t FirstErrorBlock; // Block number in the first error. 
    uint64_t FirstErrorFunc[4]; // Function where the first error occurred. 
    uint32_t FirstErrorLine; // Line number where the first error occurred. 
    uint32_t RecentErrorTime; // Most recent time an error occurred in POSIX time. 
    uint32_t LastErrorInode; // Inode number in the last error. 
    uint64_t LastErrorBlock; // Block number in the last error. 
    uint64_t LastErrorFunc[4]; // Function where the most recent error occurred. 
    uint32_t LastErrorLine; // Line number where the most recent error occurred. 
    char MountOptions; // Mount options. (C-style string: characters terminated by a 0 byte) 
    uint32_t InodeUserQuota; // Inode number for user quota file. 
    uint32_t InodeGroupQuota; // Inode number for group quota file. 
    uint32_t OverheadBlocksFS; // Overhead blocks/clusters in filesystem. Zero means the kernel calculates it at runtime. 
    uint64_t BlockGroupsBKP; // Block groups with backup Superblocks, if the sparse superblock flag is set. 
    uint8_t EncryptionAlgo[4]; // Encryption algorithms used, as a array of unsigned char. 
    uint8_t SaltS2KAlgo[16]; // Salt for the `string2key` algorithm. 
    uint32_t InodeLFDir; // Inode number of the lost+found directory. 
    uint32_t InodeProjQuotaTracker; // Inode number of the project quota tracker. 
    uint32_t CheckUUID; // Checksum of the UUID, used for the checksum seed. (crc32c(~0, UUID)) 
    uint8_t HighLastWrite; // High 8-bits of the last written time field. 
    uint8_t HighLastMount; // High 8-bits of the last mount time field. 
    uint8_t HighFSCreation; // High 8-bits of the Filesystem creation time field. 
    uint8_t HighConsistencyCheck; // High 8-bits of the last consistency check time field. 
    uint8_t HighFirstError; // High 8-bits of the first time an error occurred time field. 
    uint8_t HighLatestError; // High 8-bits of the latest time an error occurred time field. 
    uint8_t FirstErrorCode; // Error code of the first error. 
    uint8_t LatestErrorCode; // Error code of the latest error. 
    uint16_t Encoding; // Filename charset encoding. 
    uint16_t EncodingFlags; // Filename charset encoding flags. 
    uint8_t Padding[380]; // Padding.
    uint32_t Checksum; // Checksum of the superblock. 
} __attribute__((packed));

enum RequiredFeatures {
    Compression = 0x0001, // Compression is used. 
    TypeDirEnt = 0x00002, // Directory entries contain a type field. 
    RecoverData = 0x00004, // Filesystem needs to replay the Journal to recover data. 
    JournalDev = 0x00008, // Filesystem uses a journal device. 
    MetaBlockGroups = 0x00010, // Filesystem uses Meta Block Groups. 
    FileExtent = 0x00040, // Filesystem uses extents for files. 
    BITS64 = 0x00080, // Filesystem uses 64 bit features. 
    MultiMountProt = 0x00100, // Filesystem uses Multiple Mount Protection. 
    FlexBlockGroups = 0x00200, // Filesystem uses Flex Block Groups. 
    InodesExtendedAttr = 0x00400, // Filesystem uses Extended Attributes in Inodes. 
    DataDirEnt = 0x01000, // Filesystem uses Data in Directory Entries. This is not implemented as of Linux 5.9rc3. 
    MetaCheck = 0x02000, // Filesystem stores the metadata checksum seed in the superblock. This allows for changing the UUID wihtout rewriting all of the metadata blocks. 
    LargeDir = 0x04000, // Directories may be larger than 4GiB and have a maximum HTREE depth of 3. 
    InodeData = 0x08000, // Data may be stored in the inode. See Inline Data for an discussion of this feature. 
    Encryption = 0x10000, // Filesystem uses Encryption. 
    CaseFolding = 0x20000 // Filesystem uses case folding, storing the filesystem-wide encoding in inodes. 
};

namespace OptionalFeaturesEnum {
    enum OptionalFeatures {
        PreallocBlocks = 0x0001, // Preallocate some number of blocks (see byte 205 in the superblock) to a directory when creating a new one. 
        ImagicInodes = 0x0002, // Possibly unused, "imagic inodes" 
        Journal = 0x0004, // Filesystem uses a Journal 
        InodeExtendedAttributes = 0x0008, // Inodes have Extended Attributes. 
        Resizable = 0x0010, // Filesystem can resize itself for larger partitions. 
        DirHashIndex = 0x0020, // Directories use hash index. 
        Backup = 0x0200, // Backup the superblock in other block groups. 
        InodeNumFixed = 0x0800 // Inode numbers do not change during resize. 
    };
}

enum ROFeatures {
    SparseSuperBlocks = 0x0001, // Sparse superblocks and group descriptor tables
    BITS64FS = 0x0002, // File system uses a 64-bit file size 
    BinaryTree = 0x0004 // Directory contents are stored in the form of a Binary Tree
};

enum FilesystemState {
    Clean = 1, // File system is clean 
    Errors = 2 // File system has errors
};

enum ErrorHandling {
    Ignore = 1, // Ignore the error (continue on) 
    ReadOnly = 2, // Remount file system as read-only
    KernelPanic = 3 // Kernel panic 
};

enum CreatorOSIDs {
    Linux = 0,
    GNUHurd = 1,
    MASIX = 2,
    FreeBSD = 3,
    OtherLites = 4,
    AstralOS = 7 // Our Own One
};

struct FlexBlockGroupInfo {
    uint64_t Atomic64FreeClusters; // Atomic 64 bit free clusters.
    uint32_t AtomicFreeInodes; // Atomic free inodes. 
    uint32_t AtomicUsedDirs; // Atomic used directories. 
} __attribute__((packed));

struct BlockGroupDescriptor {
    uint32_t LowAddrBlockBitmap; // Low 32bits of block address of block usage bitmap. 
    uint32_t LowAddrInodeBitmap; // Low 32bits of block address of inode usage bitmap. 
    uint32_t LowAddrInodeTable; // Low 32bits of starting block address of inode table. 
    uint16_t LowUnallocBlocks; // Low 16bits of number of unallocated blocks in group. 
    uint16_t LowUnallocInodes; // Low 16bits of number of unallocated inodes in group. 
    uint16_t LowDirs; // Low 16bits of number of directories in group. 
    uint16_t Features; // Block group features present. 
    uint32_t LowAddrSnapshot; // Low 32-bits of block address of snapshot exclude bitmap. 
    uint16_t LowChkBlockBitmap; // Low 16-bits of Checksum of the block usage bitmap. 
    uint16_t LowChkInodeBitmap; // Low 16-bits of Checksum of the inode usage bitmap. 
    uint16_t LowFreeInodes; // Low 16-bits of amount of free inodes. This allows us to optimize inode searching.
    uint16_t Checksum; // Checksum of the block group, CRC16(UUID+group+desc).

    /*
     * Supported only on x64
    */
    uint32_t HighAddrBlockBitmap; // High 32-bits of block address of block usage bitmap. 
    uint32_t HighAddrInodeBitmap; // High 32-bits of block address of inode usage bitmap. 
    uint32_t HighAddrInodeTable; // High 32-bits of starting block address of inode table. 
    uint16_t HighUnallocBlocks; // High 16-bits of number of unallocated blocks in group. 
    uint16_t HighUnallocInodes; // High 16-bits of number of unallocated inodes in group. 
    uint16_t HighDirs; // High 16-bits of number of directories in group. 
    uint16_t HighFreeInodes; // High 16-bits of amount of free inodes. 
    uint32_t HighAddrSnapshot; // High 32-bits of block address of snapshot exclude bitmap
    uint16_t HighChkBlockBitmap; // High 16-bits of checksum of the block usage bitmap.
    uint16_t HighChkInodeBitmap; // High 16-bits of checksum of the inode usage bitmap
    uint32_t Reserved; // Reserved as of Linux 5.9rc3. 
} __attribute__((packed));

enum BlockGroupFlags {
    InodeUnused = 0x0001, // Block group's inode bitmap/table is unused.
    BlockUnused = 0x0002, // Block groups's block bitmap is unused.
    InodeZeroed = 0x0008 // Block groups's inode table is zeroed. 
};

struct OSVal2 {
    uint8_t FragNum; // Fragment number 
    uint8_t FragSize; // Fragment size 
    uint16_t EasterEgg; // Reserved On Linux, but an easter egg on AstralOS
    uint16_t HighUserID; // High 16 bits of 32-bit User ID 
    uint16_t HighGroupID; // High 16 bits of 32-bit Group ID
    uint32_t EasterEggChristmas; // Reserved on Linux, Christmas in ASCII
} __attribute__((packed));

struct Metadata {
    uint16_t otherExec : 1;
    uint16_t otherWrite : 1;
    uint16_t otherRead : 1;
    uint16_t groupExec : 1;
    uint16_t groupWrite : 1;
    uint16_t groupRead : 1;
    uint16_t userExec : 1;
    uint16_t userWrite : 1;
    uint16_t userRead : 1;
    uint16_t stickyBit : 1;
    uint16_t setGroupID : 1;
    uint16_t setUserID : 1;
    uint16_t Type : 4;
} __attribute__((packed));

struct Inode {
    Metadata meta; // Type and Permissions
    uint16_t UserID; // User ID (Low 16 Bits of the User ID)
    uint32_t LowerSize; // Lower 32 bits of size in bytes 
    uint32_t LastAccess; // Last Access Time (in POSIX time) 
    uint32_t Creation; // Creation Time (in POSIX time) 
    uint32_t LastModification; // Last Modification time (in POSIX time) 
    uint32_t Deletion; // Deletion time (in POSIX time) 
    uint16_t GroupID; // Group ID (Low 16 Bits of the Group ID)
    uint16_t HardLinksCount; // Count of hard links (directory entries) to this inode. When this reaches 0, the data blocks are marked as unallocated. 
    uint32_t DiskSectorCount; // Count of disk sectors (not Ext2 blocks) in use by this inode, not counting the actual inode structure nor directory entries linking to the inode. 
    uint32_t Flags; // Flags
    uint32_t OSVal1; // Operating System Specific value #1
    uint32_t DBP[15]; // Direct Block Pointer
    uint32_t Generation; // Generation number (Primarily used for NFS) 
    uint32_t ExtendedAttributeBlock; // In Ext2 version 0, this field is reserved. In version >= 1, Extended attribute block (File ACL). 
    uint32_t UpperFileSize; // In Ext2 version 0, this field is reserved. In version >= 1, Upper 32 bits of file size (if feature bit set) if it's a file, Directory ACL if it's a directory 
    uint32_t FragmentBlock; // Block address of fragment 
    OSVal2 osval2; // Operating System Specific Value #2
} __attribute__((packed));

namespace InodeTypeEnum {
    enum InodeType {
        FIFO = 0x1000, // FIFO
        CharDevice = 0x2000, // Character device
        Directory = 0x4000, // Directory
        BlockDevice = 0x6000, // Block device 
        File = 0x8000, // Regular file 
        SymLink = 0xA000, // Symbolic link 
        UnixSocket = 0xC000 // Unix socket 
    };
}

enum Permission {
    OtherExec = 0x001, // Other—execute permission
    OtherWrite = 0x002, // Other—write permission
    OtherRead = 0x004, // Other—read permission
    GroupExec = 0x008, // Group—execute permission
    GroupWrite = 0x010, // Group—write permission
    GroupRead = 0x020, // Group—read permission
    UserExec = 0x040, // User—execute permission
    UserWrite = 0x080, // User—write permission
    UserRead = 0x100, // User—read permission
    StickyBit = 0x200, // Sticky Bit
    SetGroupID = 0x400, // Set group ID 
    SetUserID = 0x800 // Set user ID 
};

enum InodeFlags {
    SecureDeletion = 0x00000001, // Secure deletion (not used) 
    DataCopy = 0x00000002, // Keep a copy of data when deleted (not used) 
    FileComp = 0x00000004, // File compression (not used) 
    SyncUpdates = 0x00000008, // Synchronous updates—new data is written immediately to disk
    ImmutableFiles = 0x00000010, // Immutable file (content cannot be changed) 
    AppendOnly = 0x00000020, // Append only
    DumpNoInc = 0x00000040, // File is not included in 'dump' command 
    FreezeTime = 0x00000080, // Last accessed time should not updated 
    HashIndexedDir = 0x00010000, // Hash indexed directory 
    AFSDir = 0x00020000, // AFS directory 
    JournalFileData = 0x00040000, // Journal file data 
    Extents = 0x00080000 // Inode uses extents
};

struct DirectoryEntry {
    uint32_t Inode; // Inode
    uint16_t TotalSize; // Total size of this entry (Including all subfields) 
    uint8_t NameLen; // Name Length least-significant 8 bits 
    uint8_t Type; // Type indicator (only if the feature bit for "directory entries have file type byte" is set, else this is the most-significant 8 bits of the Name Length) 
    char Name[]; // Size is NameLen, Name characters 
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

    virtual File* Open(FsNode* node, uint32_t flags) override;
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
    void UpdateSuperblockField(void* fieldPtr, size_t fieldSize, bool chksum);
    void UpdateGroupDesc(uint32_t group, BlockGroupDescriptor* GroupDesc);

	PartitionDevice* pdev;
	DriverServices* _ds = nullptr;
    DeviceKey devKey;

    EXT4_Superblock* superblock;
    BlockGroupDescriptor** GroupDescs;
    bool isMounted;
};