#pragma once
#include <cstdint>
#include <stddef.h>

/*

 * From Linux Kernel:
 * https://github.com/torvalds/linux/blob/master/fs/ext4/ext4.h
 * 
 * There is a new feature called Flexible Block Groups where many
 * block groups are tied together into 1 block group.
 * The first block group in the flex bg will contain the bitmaps
 * and Inodes tables of all the other bg's in the flex bg.
 * 
 * EG: flex_size = 4
 * group 0
 *  > superblock            > inode bitmaps
 *  > group descriptors     > inode tables
 *  > data blockbitmaps     > file data
 * 
 * This is so that we can load things faster by having metadata
 * close together. This also allows us to keep large files
 * continuous and not in segments.
 * 
 * This feature does not affect where the Backup Superblock's are
 * or where the group desc are, as they are always at the start of
 * block groups.
 * The number of block groups that make up a flex_bg is given by 
 * `2 ^ s_log_groups_per_flex`
*/
struct FlexBlockGroupInfo {
    uint64_t free_clusters; // Atomic 64 bit free clusters.
    uint32_t free_inodes; // Atomic free inodes. 
    uint32_t used_dirs; // Atomic used directories. 
} __attribute__((packed));

namespace BlockGroupFlags {
    enum _BlockGroupFlags {
        EXT4_BG_INODE_UNINIT = 0x0001, // Inode table/bitmap not in use
        EXT4_BG_BLOCK_UNINIT = 0x0002, // Block bitmap not in use
        EXT4_BG_INODE_ZEROED = 0x0008 // On-disk itable initialized to zero
    };
}

/*
 * From the Linux Kernel:
 * https://github.com/torvalds/linux/blob/master/fs/ext4/ext4.h
 * 
 * There are 2 types of checksums for the 
 * Block Group Descriptor in EXT4 and they
 * are easily confused.
 * 
 * The first is the GDT Checksum which is 
 * the older one that uses CRC16.
 * 
 * The second is metadata checksum which is
 * the newer one that uses CRC32C.
 * 
 * Whenever the bitmap is modified, we must
 * change the Block Bitmap or the Inode Bitmap
 * accordingly.
 * 
 * Bitmap
 *  > UUID + the entire bitmap. 
 *  > Checksums are stored in the group descriptor, 
 *    and truncated if the group descriptor size is 
 *    32 bytes (i.e. ^64bit)
 * 
 * Group Desc
 *  > If metadata_csum
 *    > UUID + group number + the entire descriptor
 *  > else if gdt_csum
 *    > crc16(UUID + group number + the entire descriptor). 
 *  > In all cases, only the lower 16 bits are stored.
*/
struct BlockGroupDescriptor {
    uint32_t bg_block_bitmap_lo; // Blocks bitmap block
    uint32_t bg_inode_bitmap_lo; // Inodes bitmap block
    uint32_t bg_inode_table_lo; // Inodes table block
    uint16_t bg_free_blocks_count_lo; // Free blocks count
    uint16_t bg_free_inodes_count_lo; // Free inodes count
    uint16_t bg_used_dirs_count_lo; // Directories count
    uint16_t bg_flags; // EXT4_BG_flags (INODE_UNINIT, etc)
    uint32_t bg_exclude_bitmap_lo; // Exclude bitmap for snapshots
    uint16_t bg_block_bitmap_csum_lo; // crc32c(s_uuid+grp_num+bbitmap) LE
    uint16_t bg_inode_bitmap_csum_lo; // crc32c(s_uuid+grp_num+ibitmap) LE 
    uint16_t bg_itable_unused_lo; // Unused inodes count
    uint16_t bg_checksum; // crc16(sb_uuid+group+desc)

    /*
     * Supported only on x64
    */
    uint32_t bg_block_bitmap_hi; // Blocks bitmap block MSB
    uint32_t bg_inode_bitmap_hi; // Inodes bitmap block MSB
    uint32_t bg_inode_table_hi; // Inodes table block MSB 
    uint16_t bg_free_blocks_count_hi; // Free blocks count MSB
    uint16_t bg_free_inodes_count_hi; // Free inodes count MSB
    uint16_t bg_used_dirs_count_hi; // Directories count MSB
    uint16_t bg_itable_unused_hi; // Unused inodes count MSB
    uint32_t bg_exclude_bitmap_hi; // Exclude bitmap block MSB
    uint16_t bg_block_bitmap_csum_hi; // crc32c(s_uuid+grp_num+bbitmap) BE
    uint16_t bg_inode_bitmap_csum_hi; // crc32c(s_uuid+grp_num+ibitmap) BE
    uint32_t bg_reserved; // Reserved as of Linux 5.9rc3. 
} __attribute__((packed));