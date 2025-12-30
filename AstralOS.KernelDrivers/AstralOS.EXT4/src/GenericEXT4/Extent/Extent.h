#pragma once
#include <cstdint>
#include <stddef.h>

struct ExtentHeader {
    uint16_t eh_magic; // Magic number, 0xF30A.
    uint16_t eh_entries; // Number of valid entries following the header.
    uint16_t eh_max; // Maximum number of entries that could follow the header.
    uint16_t eh_depth; // Depth of this extent node in the extent tree. 0 = this extent node points to data blocks; otherwise, this extent node points to other extent nodes. The extent tree can be at most 5 levels deep: a logical block number can be at most 2^32, and the smallest n that satisfies 4*(((blocksize - 12)/12)^n) >= 2^32 is 5.
    uint32_t eh_generation; // Generation of the tree. (Used by Lustre, but not standard ext4).
} __attribute__((packed));

struct Extent {
    uint32_t ee_block; // First file block number that this extent covers.
    uint16_t ee_len; // Number of blocks covered by extent. If the value of this field is <= 32768, the extent is initialized. If the value of the field is > 32768, the extent is uninitialized and the actual extent length is ee_len - 32768. Therefore, the maximum length of a initialized extent is 32768 blocks, and the maximum length of an uninitialized extent is 32767.
    uint16_t ee_start_hi; // Upper 16-bits of the block number to which this extent points.
    uint32_t ee_start_lo; // Lower 32-bits of the block number to which this extent points.
} __attribute__((packed));

struct ExtentIDX {
    uint32_t ei_block; // This index node covers file blocks from 'block' onward.
    uint32_t ei_leaf_lo; // Lower 32-bits of the block number of the extent node that is the next level lower in the tree. The tree node pointed to can be either another internal node or a leaf node, described below.
    uint16_t ei_leaf_hi; // Upper 16-bits of the previous field.
    uint16_t ei_unused;
} __attribute__((packed));

struct ExtentTail {
    uint32_t eb_checksum; // Checksum of the extent block, crc32c(uuid+inum+igeneration+extentblock)
} __attribute__((packed));