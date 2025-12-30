#pragma once
#include <cstdint>
#include <stddef.h> 

#define EXT4_NAME_LEN 255

/*
 * These structs are from the Linux Kernel:
 * https://github.com/torvalds/linux/blob/master/fs/ext4/ext4.h
*/
struct DirectoryEntry {
    uint32_t inode; // Inode number
    uint16_t rec_len; // Directory entry length
    uint8_t name_len; // Name length 
    char Name[EXT4_NAME_LEN]; // File name
} __attribute__((packed));

/*
 * Encrypted Casefolded entries require saving the hash on disk. This structure
 * followed ext4_dir_entry_2's name[name_len] at the next 4 byte aligned
 * boundary.
 */
struct DirectoryEntryHash {
	uint32_t hash;
	uint32_t minor_hash;
} __attribute__((packed));

struct DirectoryEntry2 {
	uint32_t inode; // Inode number 
	uint16_t rec_len; // Directory entry length 
	uint8_t name_len; // Name length 
	uint8_t file_type; // See file type macros EXT4_FT_* below 
	char name[EXT4_NAME_LEN]; // File name 
};

struct DirectoryEntryTail {
    uint32_t det_reserved_zero1; // Pretend to be unused
    uint16_t det_rec_len; // 12
    uint8_t det_reserved_zero2; // Zero name length
    uint8_t det_reserved_ft; // 0xDE, fake file type
    uint32_t det_checksum; // crc32c(uuid+inum+dirblock)
} __attribute__((packed));

namespace DirectoryEntryType {
    enum _DirectoryEntryType {
        EXT4_FT_UNKNOWN = 0,
        EXT4_FT_REG_FIL = 1,
        EXT4_FT_DIR = 2,
        EXT4_FT_CHRDEV = 3,
        EXT4_FT_BLKDEV = 4,
        EXT4_FT_FIFO = 5,
        EXT4_FT_SOCK = 6,
        EXT4_FT_SYMLINK = 7,
        EXT4_FT_MAX = 8,
        EXT4_FT_DIR_CSUM = 0xDE,
    };
}