#pragma once
#include <cstdint>
#include <stddef.h>

/*
 * From Linux Kernel:
 * https://github.com/torvalds/linux/blob/master/Documentation/filesystems/ext4/inodes.rst
 * 
 * EXT4 uses Inodes, also called Index Nodes, to store all the metadata for the file, dir,
 * etc. The Inode points to the Directory Entry which holds information like the name, etc.
 * Unlike other Filesystems, EXT4 stores the filetype in the Inode.
*/
struct Inode {
    uint16_t i_mode; // Type and Permissions
    uint16_t i_uid; // User ID (Low 16 Bits of the User ID)
    uint32_t i_size_lo; // Lower 32 bits of size in bytes 
    uint32_t i_atime; // Last Access Time (in POSIX time) 
    uint32_t i_ctime; // Last Change Time (in POSIX time) 
    uint32_t i_mtime; // Last Modification time (in POSIX time) 
    uint32_t i_dtime; // Deletion time (in POSIX time) 
    uint16_t i_gid; // Group ID (Low 16 Bits of the Group ID)
    uint16_t i_links_count; // Count of hard links (directory entries) to this inode. When this reaches 0, the data blocks are marked as unallocated. 
    uint32_t i_blocks_lo; // Lower Blocks Count
    uint32_t i_flags; // Flags
    union {
		struct {
			uint32_t  l_i_version;
		} linux1;
		struct {
			uint32_t  h_i_translator;
		} hurd1;
		struct {
			uint32_t  m_i_reserved1;
		} masix1;
	} i_osd1; // Operating System Specific value #1
    uint32_t i_block[15]; // Direct Block Pointer
    uint32_t i_generation; // Generation number (Primarily used for NFS) 
    uint32_t i_file_acl_lo; // In Ext2 version 0, this field is reserved. In version >= 1, Extended attribute block (File ACL). 
    uint32_t i_size_high; // In Ext2 version 0, this field is reserved. In version >= 1, Upper 32 bits of file size (if feature bit set) if it's a file, Directory ACL if it's a directory 
    uint32_t i_obso_faddr; // Block address of fragment 
    union {
		struct {
			uint16_t l_i_blocks_high; // were l_i_reserved1
			uint16_t l_i_file_acl_high;
			uint16_t l_i_uid_high; // these 2 fields
			uint16_t l_i_gid_high; // were reserved2[0]
			uint16_t l_i_checksum_lo; // crc32c(uuid+inum+inode) LE
			uint16_t l_i_reserved;
		} linux2;
		struct {
			uint16_t h_i_reserved1;	// Obsoleted fragment number/size which are removed in ext4
			uint16_t h_i_mode_high;
			uint16_t h_i_uid_high;
			uint16_t h_i_gid_high;
			uint32_t h_i_author;
		} hurd2;
		struct {
			uint16_t h_i_reserved1;	// Obsoleted fragment number/size which are removed in ext4
			uint16_t m_i_file_acl_high;
			uint32_t m_i_reserved2[2];
		} masix2;
        struct {
			uint16_t a_i_blocks_high; // were l_i_reserved1
			uint16_t a_i_file_acl_high;
			uint16_t a_i_uid_high; // these 2 fields
			uint16_t a_i_gid_high; // were reserved2[0]
			uint16_t a_i_checksum_lo; // crc32c(uuid+inum+inode) LE
			uint16_t a_i_easteregg;
		} astral2;
	} i_osd2; // Operating System Specific Value #2
    uint16_t i_extra_isize; // High 16 bits of file size (if feature bit set)
    uint16_t i_checksum_hi; // High 16 bits of the Checksum
    uint32_t i_ctime_extra; // Extra last change time field? (in POSIX time) 
    uint32_t i_mtime_extra; // Extra last modification (in POSIX time) 
    uint32_t i_atime_extra; // Extra Last Access (in POSIX time) 
    uint32_t i_crtime; // File Creation (in POSIX time) 
    uint32_t i_crtime_extra; // Extra file creation time bits. This provides sub-second precision.
    uint32_t i_version_hi; // Upper 32-bits for version number.
    uint32_t i_projid; // Project ID.
} __attribute__((packed));

static_assert(offsetof(Inode, i_projid) == 0x9C, "Project ID is in the wrong place");

namespace InodeMode {
    enum _InodeMode {
        S_IXOTH = 0x1, // Others may execute
        S_IWOTH = 0x2, // Others may write
        S_IROTH = 0x4, // Others may read
        S_IXGRP = 0x8, // Group members may execute
        S_IWGRP = 0x10, // Group members may write
        S_IRGRP = 0x20, // Group members may read
        S_IXUSR = 0x40, // Owner may execute
        S_IWUSR = 0x80, // Owner may write
        S_IRUSR = 0x100, // Owner may read
        S_ISVTX = 0x200, // Sticky bit
        S_ISGID = 0x400, // Set GID
        S_ISUID = 0x800, // Set UID
        S_IFIFO = 0x1000, // FIFO
        S_IFCHR = 0x2000, // Character device
        S_IFDIR = 0x4000, // Directory
        S_IFBLK = 0x6000, // Block device
        S_IFREG = 0x8000, // Regular file
        S_IFLNK = 0xA000, // Symbolic link
        S_IFSOCK = 0xC000, // Socket
    };
}

namespace InodeFlags {
    enum _InodeFlags {
        EXT4_SECRM_FL = 0x1, // This file requires secure deletion
        EXT4_UNRM_FL = 0x2, // This file should be preserved, should undeletion be desired
        EXT4_COMPR_FL = 0x4, // File is compressed
        EXT4_SYNC_FL = 0x8, // All writes to the file must be synchronous
        EXT4_IMMUTABLE_FL = 0x10, // File is immutable
        EXT4_APPEND_FL = 0x20, // File can only be appended
        EXT4_NODUMP_FL = 0x40, // The dump(1) utility should not dump this file
        EXT4_NOATIME_FL = 0x80, // Do not update access time
        EXT4_DIRTY_FL = 0x100, // Dirty compressed file
        EXT4_COMPRBLK_FL = 0x200, // File has one or more compressed clusters
        EXT4_NOCOMPR_FL = 0x400, // Do not compress file
        EXT4_ENCRYPT_FL = 0x800, // Encrypted inode. This bit value previously was EXT4_ECOMPR_FL (compression error), which was never used.
        EXT4_INDEX_FL = 0x1000, // Directory has hashed indexes
        EXT4_IMAGIC_FL = 0x2000, // AFS magic directory
        EXT4_JOURNAL_DATA_FL = 0x4000, // File data must always be written through the journal
        EXT4_NOTAIL_FL = 0x8000, // File tail should not be merged. (not used by ext4)
        EXT4_DIRSYNC_FL = 0x10000, // All directory entry data should be written synchronously
        EXT4_TOPDIR_FL = 0x20000, // Top of directory hierarchy
        EXT4_HUGE_FILE_FL = 0x40000, // This is a huge file
        EXT4_EXTENTS_FL = 0x80000, // Inode uses extents
        EXT4_VERITY_FL = 0x100000, // Verity protected file
        EXT4_EA_INODE_FL = 0x200000, // Inode stores a large extended attribute value in its data blocks
        EXT4_EOFBLOCKS_FL = 0x400000, // This file has blocks allocated past EOF
        EXT4_SNAPFILE_FL = 0x01000000, // Inode is a snapshot
        EXT4_SNAPFILE_DELETED_FL = 0x04000000, // Snapshot is being deleted
        EXT4_SNAPFILE_SHRUNK_FL = 0x08000000, // Snapshot shrink has completed
        EXT4_INLINE_DATA_FL = 0x10000000, // Inode has inline data
        EXT4_PROJINHERIT_FL = 0x20000000, // Create children with the same project ID
        EXT4_CASEFOLD_FL = 0x40000000, // Use case-insensitive lookups for directory contents
        EXT4_RESERVED_FL = 0x80000000, // Reserved for ext4 library
        EXT4_USERVISIBLE_FL = 0x705BDFFF, // User-visible flags.
        EXT4_USERMOD_FL = 0x604BC0FF, // User-modifiable flags. Note that while EXT4_JOURNAL_DATA_FL and EXT4_EXTENTS_FL can be set with setattr, they are not in the kernel's EXT4_FL_USER_MODIFIABLE mask, since it needs to handle the setting of these flags in a special manner and they are masked out of the set of flags that are saved directly to i_flags.
    };
}