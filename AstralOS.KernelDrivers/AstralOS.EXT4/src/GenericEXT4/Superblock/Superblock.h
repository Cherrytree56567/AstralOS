#pragma once
#include <cstdint>
#include <stddef.h>

/*
 * -This struct is inspired by the OSDev Wiki:
 * https://wiki.osdev.org/Ext4-
 * 
 * The struct is now based off the IncoreSemi
 * Gitlab Page: 
 * https://gitlab.incoresemi.com/software/linux/-/blob/0e698dfa282211e414076f9dc7e83c1c288314fd/Documentation/filesystems/ext4/super.rst
 * 
 * Turns out the Linux Kernel Repo also has docs
 * so if you want some more up to date info, its
 * here:
 * https://github.com/torvalds/linux/blob/master/Documentation/filesystems/ext4/super.rst
 * 
 * However the info below if based on the EXT2
 * page on the OSDev Wiki and is considered
 * highly inaccurate for EXT4.
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
    uint32_t s_inodes_count; // Total inode count.
    uint32_t s_blocks_count_lo; // Total block count.
    uint32_t s_r_blocks_count_lo; // This number of blocks can only be allocated by the super-user.
    uint32_t s_free_blocks_count_lo; // Free block count.
    uint32_t s_free_inodes_count; // Free inode count.
    uint32_t s_first_data_block; // First data block. This must be at least 1 for 1k-block filesystems and is typically 0 for all other block sizes.
    uint32_t s_log_block_size; // Block size is 2 ^ (10 + s_log_block_size).
    uint32_t s_log_cluster_size; // Cluster size is 2 ^ (10 + s_log_cluster_size) blocks if bigalloc is enabled. Otherwise s_log_cluster_size must equal s_log_block_size.
    uint32_t s_blocks_per_group; // Blocks per group.
    uint32_t s_clusters_per_group; // Clusters per group, if bigalloc is enabled. Otherwise s_clusters_per_group must equal s_blocks_per_group.
    uint32_t s_inodes_per_group; // Inodes per group.
    uint32_t s_mtime; // Mount time, in seconds since the epoch.
    uint32_t s_wtime; // Write time, in seconds since the epoch.
    uint16_t s_mnt_count; // Number of mounts since the last fsck.
    uint16_t s_max_mnt_count; // Number of mounts beyond which a fsck is needed.
    uint16_t s_magic; // Magic signature, 0xEF53
    uint16_t s_state; // File system state.
    uint16_t s_errors; // Behaviour when detecting errors.
    uint16_t s_minor_rev_level; // Minor revision level.
    uint32_t s_lastcheck; // Time of last check, in seconds since the epoch.
    uint32_t s_checkinterval; // Maximum time between checks, in seconds.
    uint32_t s_creator_os; // Creator OS.
    uint32_t s_rev_level; // Revision level.
    uint16_t s_def_resuid; // Default uid for reserved blocks.
    uint16_t s_def_resgid; // Default gid for reserved blocks.

    /*
     * For Dynamic Superblocks Only
     *
     * These fields are for EXT4_DYNAMIC_REV superblocks only.
     * Note: the difference between the compatible feature set 
     * and the incompatible feature set is that if there is a 
     * bit set in the incompatible feature set that the kernel 
     * doesn't know about, it should refuse to mount the extfs.
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible 
     * feature set, it must abort and not try to meddle with things 
     * it doesn't understand...
    */
    uint32_t s_first_ino; // First non-reserved inode.
    uint16_t s_inode_size; // Size of inode structure, in bytes.
    uint16_t s_block_group_nr; // Block group # of this superblock.
    uint32_t s_feature_compat; // Compatible feature set flags. Kernel can still read/write this fs even if it doesn't understand a flag; fsck should not do that.
    uint32_t s_feature_incompat; // Incompatible feature set. If the kernel or fsck doesn't understand one of these bits, it should stop.
    uint32_t s_feature_ro_compat; // Readonly-compatible feature set. If the kernel doesn't understand one of these bits, it can still mount read-only.
    uint8_t s_uuid[16]; // 128-bit UUID for volume.
    char s_volume_name[16]; // Volume label.
    char s_last_mounted[64]; // Directory where filesystem was last mounted. 
    uint32_t s_algorithm_usage_bitmap; // For compression (Not used in e2fsprogs/Linux)
    /*
     * Performance hints.  Directory preallocation should only happen if thE
     * EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
    */
    uint8_t s_prealloc_blocks; // #. of blocks to try to preallocate for ... files? (Not used in e2fsprogs/Linux)
    uint8_t s_prealloc_dir_blocks; // #. of blocks to preallocate for directories. (Not used in e2fsprogs/Linux)
    uint16_t s_reserved_gdt_blocks; // Number of reserved GDT entries for future filesystem expansion.
    /*
     * Journalling support is valid only if EXT4_FEATURE_COMPAT_HAS_JOURNAL is set.
    */
    uint8_t s_journal_uuid[16]; // UUID of journal superblock
    uint32_t s_journal_inum; // inode number of journal file. 
    uint32_t s_journal_dev; // Device number of journal file, if the external journal feature flag is set.
    uint32_t s_last_orphan; // Start of list of orphaned inodes to delete
    uint32_t s_hash_seed[4]; // HTREE hash seed.
    uint8_t s_def_hash_version; // Default hash algorithm to use for directory hashes. 
    uint8_t s_jnl_backup_type; // If this value is 0 or EXT3_JNL_BACKUP_BLOCKS (1), then the s_jnl_blocks field contains a duplicate copy of the inode's i_block[] array and i_size.
    uint16_t s_desc_size; // Size of group descriptors, in bytes, if the 64bit incompat feature flag is set.
    uint32_t s_default_mount_opts; // Default mount options.
    uint32_t s_first_meta_bg; // First metablock block group, if the meta_bg feature is enabled.
    uint32_t s_mkfs_time; // When the filesystem was created, in seconds since the epoch.
    uint32_t s_jnl_blocks[17]; // Backup copy of the journal inode's i_block[] array in the first 15 elements and i_size_high and i_size in the 16th and 17th elements, respectively.

    /*
     * 64-Bit Support
     * 
     * 64bit support is valid only if EXT4_FEATURE_COMPAT_64BIT is set.
    */
    uint32_t s_blocks_count_hi; // High 32-bits of the block count.
    uint32_t s_r_blocks_count_hi; // High 32-bits of the reserved block count.
    uint32_t s_free_blocks_count_hi; // High 32-bits of the free block count.
    uint16_t s_min_extra_isize; // All inodes have at least # bytes.
    uint16_t s_want_extra_isize; // New inodes should reserve # bytes. 
    uint32_t s_flags; // Miscellaneous flags.
    uint16_t s_raid_stride; // RAID stride. This is the number of logical blocks read from or written to the disk before moving to the next disk. This affects the placement of filesystem metadata, which will hopefully make RAID storage faster.
    uint16_t s_mmp_interval; // #. seconds to wait in multi-mount prevention (MMP) checking. In theory, MMP is a mechanism to record in the superblock which host and device have mounted the filesystem, in order to prevent multiple mounts. This feature does not seem to be implemented
    uint64_t s_mmp_block; // Block # for multi-mount protection data.
    uint32_t s_raid_stripe_width; // RAID stripe width. This is the number of logical blocks read from or written to the disk before coming back to the current disk. This is used by the block allocator to try to reduce the number of read-modify-write operations in a RAID5/6.
    uint8_t s_log_groups_per_flex; // Size of a flexible block group is 2 ^ s_log_groups_per_flex.
    uint8_t s_checksum_type; // Metadata checksum algorithm type. The only valid value is 1 (crc32c).
    uint8_t s_encryption_level; // Encryption version level.  Not in the IncoreSemi Repo??
    uint8_t s_reserved_pad; // Reserved padding. 
    uint64_t s_kbytes_written; // Number of KiB written to this filesystem over its lifetime.
    uint32_t s_snapshot_inum; // inode number of active snapshot. (Not used in e2fsprogs/Linux.)
    uint32_t s_snapshot_id; // Sequential ID of active snapshot. (Not used in e2fsprogs/Linux.)
    uint64_t s_snapshot_r_blocks_count; // Number of blocks reserved for active snapshot's future use. (Not used in e2fsprogs/Linux.)
    uint32_t s_snapshot_list; // inode number of the head of the on-disk snapshot list. (Not used in e2fsprogs/Linux.)
    uint32_t s_error_count; // Number of errors seen.
    uint32_t s_first_error_time; // First time an error happened, in seconds since the epoch.
    uint32_t s_first_error_ino; // inode involved in first error.
    uint64_t s_first_error_block; // Number of block involved of first error.
    uint8_t s_first_error_func[32]; // Name of function where the error happened.
    uint32_t s_first_error_line; // Line number where error happened.
    uint32_t s_last_error_time; // Time of most recent error, in seconds since the epoch.
    uint32_t s_last_error_ino; // inode involved in most recent error.
    uint32_t s_last_error_line; // Line number where most recent error happened.
    uint64_t s_last_error_block; // Number of block involved in most recent error.
    uint8_t s_last_error_func[32]; // Name of function where the most recent error happened.
    uint8_t s_mount_opts[64]; // ASCIIZ string of mount options.
    uint32_t s_usr_quota_inum; // Inode number of user quota file.
    uint32_t s_grp_quota_inum; // Inode number of group quota file.
    uint32_t s_overhead_blocks; // Overhead blocks/clusters in fs. (Huh This field is always zero, which means that the kernel calculates it dynamically.)
    uint32_t s_backup_bgs[2]; // Block groups containing superblock backups (if sparse_super2)
    uint8_t s_encrypt_algos[4]; // Encryption algorithms in use. There can be up to four algorithms in use at any time
    uint8_t s_encrypt_pw_salt[16]; // Salt for the string2key algorithm for encryption.
    uint32_t s_lpf_ino; // Inode number of lost+found
    uint32_t s_prj_quota_inum; // Inode that tracks project quotas.
    uint32_t s_checksum_seed; // Checksum seed used for metadata_csum calculations. This value is crc32c(~0, $orig_fs_uuid). 
    uint8_t s_wtime_hi; // Upper 8 bits of the s_wtime field.
    uint8_t s_mtime_hi; // Upper 8 bits of the s_mtime field.
    uint8_t s_mkfs_time_hi; // Upper 8 bits of the s_mkfs_time field.
    uint8_t s_lastcheck_hi; // Upper 8 bits of the s_lastcheck_hi field.
    uint8_t s_first_error_time_hi; // Upper 8 bits of the s_first_error_time_hi field.
    uint8_t s_last_error_time_hi; // Upper 8 bits of the s_last_error_time_hi field.
    uint8_t s_first_error_errcode; // 
    uint8_t s_last_error_errcode; // 
    uint16_t s_encoding; // Filename charset encoding.
    uint16_t s_encoding_flags; // Filename charset encoding flags.
    uint32_t s_orphan_file_inum; // Orphan file inode number.
    uint32_t s_reserved[94]; // Padding to the end of the block.
    uint32_t s_checksum; // Superblock checksum.
} __attribute__((packed));

static_assert(offsetof(EXT4_Superblock, s_checksum) == 0x3FC, "Checksum is in the wrong place");
static_assert(sizeof(EXT4_Superblock) == 1024, "Superblock size must be 1024 bytes");

namespace SuperblockState {
    enum _SuperblockState {
        Clean = 0x0001, // Cleanly umounted
        Errors = 0x0002, // Errors detected
        Recovering = 0x0004, // Orphans being recovered
    };
}

namespace ErrorPolicy {
    enum _ErrorPolicy {
        Continue = 1, // Ignore the error (continue on) 
        ReadOnly = 2, // Remount file system as read-only
        Panic = 3 // Kernel panic 
    };
}

namespace CreatorOSIDs {
    enum _CreatorOSIDs {
        Linux = 0,
        GNUHurd = 1,
        MASIX = 2,
        FreeBSD = 3,
        OtherLites = 4,
        AstralOS = 7 // Our Own One
    };
}

/*
 * EXT4_DYNAMIC_REV refers to a revision 1 or newer filesystem.
*/
namespace SuperblockRevision {
    enum _SuperblockRevision {
        Original = 0, // Original format
        V2Dynamic = 1 // v2 format w/ dynamic inode sizes
    };
}

namespace CompatibleFeatures {
    enum _CompatibleFeatures {
        COMPAT_DIR_PREALLOC = 0x1, // Directory preallocation
        COMPAT_IMAGIC_INODES = 0x2, // “imagic inodes”. Not clear from the code what this does
        COMPAT_HAS_JOURNAL = 0x4, // Has a journal
        COMPAT_EXT_ATTR = 0x8, // Supports extended attributes
        COMPAT_RESIZE_INODE = 0x10, // Has reserved GDT blocks for filesystem expansion. Requires RO_COMPAT_SPARSE_SUPER.
        COMPAT_DIR_INDEX = 0x20, // Has directory indices 
        COMPAT_LAZY_BG = 0x40, // “Lazy BG”. Not in Linux kernel, seems to have been for uninitialized block groups?
        COMPAT_EXCLUDE_INODE = 0x80, // “Exclude inode”. Not used.
        COMPAT_EXCLUDE_BITMAP = 0x100, // “Exclude bitmap”. Seems to be used to indicate the presence of snapshot-related exclude bitmaps? Not defined in kernel or used in e2fsprogs
        COMPAT_SPARSE_SUPER2 = 0x200, // Sparse Super Block, v2. If this flag is set, the SB field s_backup_bgs points to the two block groups that contain backup superblocks
        COMPAT_FAST_COMMIT = 0x400, // Fast commits supported. Although fast commits blocks are backward incompatible, fast commit blocks are not always present in the journal. If fast commit blocks are present in the journal, JBD2 incompat feature (JBD2_FEATURE_INCOMPAT_FAST_COMMIT) gets set
        COMPAT_ORPHAN_PRESENT = 0x1000, // Orphan file allocated. This is the special file for more efficient tracking of unlinked but still open inodes. When there may be any entries in the file, we additionally set proper rocompat feature
    };
}

namespace IncompatFeatures {
    enum _IncompatFeatures {
        INCOMPAT_COMPRESSION = 0x1, // Compression
        INCOMPAT_FILETYPE = 0x2, // Directory entries record the file type.
        INCOMPAT_RECOVER = 0x4, // Filesystem needs recovery
        INCOMPAT_JOURNAL_DEV = 0x8, // Filesystem has a separate journal device 
        INCOMPAT_META_BG = 0x10, // Meta block groups.
        INCOMPAT_EXTENTS = 0x40, // Files in this filesystem use extents 
        INCOMPAT_64BIT = 0x80, // Enable a filesystem size of 2^64 blocks
        INCOMPAT_MMP = 0x100, // Multiple mount protection
        INCOMPAT_FLEX_BG = 0x200, // Flexible block groups.
        INCOMPAT_EA_INODE = 0x400, // Inodes can be used to store large extended attribute values
        INCOMPAT_DIRDATA = 0x1000, // Data in directory entry. This is not implemented as of Linux 5.9rc3. 
        INCOMPAT_CSUM_SEED = 0x2000, // Metadata checksum seed is stored in the superblock. This feature enables the administrator to change the UUID of a metadata_csum filesystem while the filesystem is mounted; without it, the checksum definition requires all metadata blocks to be rewritten 
        INCOMPAT_LARGEDIR = 0x4000, // Large directory >2GB or 3-level htree. Prior to this feature, directories could not be larger than 4GiB and could not have an htree more than 2 levels deep. If this feature is enabled, directories can be larger than 4GiB and have a maximum htree depth of 3.
        INCOMPAT_INLINE_DATA = 0x8000, // Data in inode
        INCOMPAT_ENCRYPT = 0x10000, // Encrypted inodes are present on the filesystem. 
        INCOMPAT_CASEFOLD = 0x20000, // Directories can be marked case-insensitive.
    };
}

namespace ROFeatures {
    enum _ROFeatures {
        RO_COMPAT_SPARSE_SUPER = 0x1, // Sparse superblocks.
        RO_COMPAT_LARGE_FILE = 0x2, // This filesystem has been used to store a file greater than 2GiB
        RO_COMPAT_BTREE_DIR = 0x4, // Not used in kernel or e2fsprogs
        RO_COMPAT_HUGE_FILE = 0x8, // This filesystem has files whose sizes are represented in units of logical blocks, not 512-byte sectors. This implies a very large file indeed!
        RO_COMPAT_GDT_CSUM = 0x10, // Group descriptors have checksums. In addition to detecting corruption, this is useful for lazy formatting with uninitialized groups
        RO_COMPAT_DIR_NLINK = 0x20, // Indicates that the old ext3 32,000 subdirectory limit no longer applies. A directory's i_links_count will be set to 1 if it is incremented past 64,999.
        RO_COMPAT_EXTRA_ISIZE = 0x40, // Indicates that large inodes exist on this filesystem
        RO_COMPAT_HAS_SNAPSHOT = 0x80, // This filesystem has a snapshot 
        RO_COMPAT_QUOTA = 0x100, // Quota
        RO_COMPAT_BIGALLOC = 0x200, // This filesystem supports “bigalloc”, which means that file extents are tracked in units of clusters (of blocks) instead of blocks
        RO_COMPAT_METADATA_CSUM = 0x400, // This filesystem supports metadata checksumming. (implies RO_COMPAT_GDT_CSUM, though GDT_CSUM must not be set)
        RO_COMPAT_REPLICA = 0x800, // Filesystem supports replicas. This feature is neither in the kernel nor e2fsprogs.
        RO_COMPAT_READONLY = 0x1000, // Read-only filesystem image; the kernel will not mount this image read-write and most tools will refuse to write to the image.
        RO_COMPAT_PROJECT = 0x2000, // Filesystem tracks project quotas.
        RO_COMPAT_VERITY = 0x8000, // Verity inodes may be present on the filesystem.
        RO_COMPAT_ORPHAN_PRESENT = 0x10000, // Indicates orphan file may have valid orphan entries and thus we need to clean them up when mounting the filesystem
    };
}

namespace HashVersion {
    enum _HashVersion {
        Legacy = 0x0,
        HalfMD4 = 0x1,
        Tea = 0x2,
        LegacyUnsigned = 0x3,
        HalfMD4Unsigned = 0x4,
        TeaUnsigned = 0x5,
    };
}

namespace MountOptions {
    enum _MountOptions {
        EXT4_DEFM_DEBUG = 0x0001, // Print debugging info upon (re)mount.
        EXT4_DEFM_BSDGROUPS = 0x0002, // New files take the gid of the containing directory (instead of the fsgid of the current process).
        EXT4_DEFM_XATTR_USER = 0x0004, // Support userspace-provided extended attributes.
        EXT4_DEFM_ACL = 0x0008, // Support POSIX access control lists (ACLs).
        EXT4_DEFM_UID16 = 0x0010, // Do not support 32-bit UIDs.
        EXT4_DEFM_JMODE_DATA = 0x0020, // All data and metadata are commited to the journal.
        EXT4_DEFM_JMODE_ORDERED = 0x0040, // All data are flushed to the disk before metadata are committed to the journal.
        EXT4_DEFM_JMODE_WBACK = 0x0060, // Data ordering is not preserved; data may be written after the metadata has been written.
        EXT4_DEFM_NOBARRIER = 0x0100, // Disable write flushes.
        EXT4_DEFM_BLOCK_VALIDITY = 0x0200, // Track which blocks in a filesystem are metadata and therefore should not be used as data blocks. This option will be enabled by default on 3.18, hopefully.
        EXT4_DEFM_DISCARD = 0x0400, // Enable DISCARD support, where the storage device is told about blocks becoming unused.
        EXT4_DEFM_NODELALLOC = 0x0800, // Disable delayed allocation.
    };
}

namespace SuperblockFlags {
    enum _SuperblockFlags {
        EXT4_SIGNED_DIR = 0x0, // Signed directory hash in use.
        EXT4_UNSIGNED_DIR = 0x0, // Unsigned directory hash in use.
        EXT4_TESTING = 0x0, // To test development code.
    };
}

namespace EncryptAlgos {
    enum _EncryptAlgos {
        ENCRYPTION_MODE_INVALID = 0, // Invalid algorithm
        ENCRYPTION_MODE_AES_256_XTS = 1, // 256-bit AES in XTS mode
        ENCRYPTION_MODE_AES_256_GCM = 2, // 256-bit AES in GCM mode
        ENCRYPTION_MODE_AES_256_CBC = 3, // 256-bit AES in CBC mode
    };
}
