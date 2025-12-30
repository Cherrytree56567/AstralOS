#pragma once
#include <cstdint>
#include <stddef.h>

/*
 * From the Linux Kernel:
 * https://github.com/torvalds/linux/blob/8640b74557fc8b4c300030f6ccb8cd078f665ec8/fs/ext4/ext4.h#L2693
*/
#define EXT4_MMP_MAGIC     0x004D4D50U /* ASCII for MMP */
#define EXT4_MMP_SEQ_CLEAN 0xFF4D4D50U /* mmp_seq value for clean unmount */
#define EXT4_MMP_SEQ_FSCK  0xE24D4D50U /* mmp_seq value when being fscked */
#define EXT4_MMP_SEQ_MAX   0xE24D4D4FU /* maximum valid mmp_seq value */

/*
 * MMP is a Feature on EXT4 that prevents the fs from being
 * mounted multiple times. You must first check if the seq
 * num is EXT4_MMP_SEQ_CLEAN, to mount. If its MMP_SEQ_FSCK
 * then it means that fsck is running and that you shouldn't
 * mount. Otherwise, you must wait for twice the MMP check
 * interval (in seconds) and check the sequence number again
 * If the sequence num has changed, then the fs is active on
 * another machine, and you shouldn't mount. If the MMP code
 * passes all the checks, a new MMP sequence num is generated
 * and written to memory and the fs is mounted. While the fs
 * is mounted, the kernel sets up a timer to recheck the MMP
 * block at the internal. The MMP sequence num is re-read and
 * if it doesn't match the in-memory MMP sequence number then
 * it means that another device has mounted it, so the fs is
 * then remounted as read-only. If both sequence numbers are
 * the same, then both (in-memory and on-disk) are increased
 * 
*/
struct MMP {
    uint32_t mmp_magic; // Magic number for MMP, 0x004D4D50 (“MMP”).
    uint32_t mmp_seq; // Sequence number, updated periodically.
    uint64_t mmp_time; // Time that the MMP block was last updated.
    char mmp_nodename[64]; // Hostname of the node that opened the filesystem.
    char mmp_bdevname[32]; // Block device name of the filesystem.
    uint16_t mmp_check_interval; // The MMP re-check interval, in seconds.
    uint16_t mmp_pad1; // Zero.
    uint32_t mmp_pad2[226]; // Zero.
    uint32_t mmp_checksum; // Checksum of the MMP block.
} __attribute__((packed));