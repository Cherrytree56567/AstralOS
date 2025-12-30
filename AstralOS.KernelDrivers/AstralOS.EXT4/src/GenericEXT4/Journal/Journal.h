#pragma once
#include <cstdint>
#include <stddef.h>

struct Journal {
    uint32_t h_magic; // Magic signature (0xc03b3998) 
    uint32_t h_blocktype; // Block Type
    uint32_t h_sequence; // Journal transaction for this block.
} __attribute__((packed));

namespace BlockType {
    enum _BlockType {
        Descriptor = 1, // This block precedes a series of data blocks that were written through the journal during a transaction.
        BlockCommitRecord = 2, // This block signifies the completion of a transaction.
        JournalSuperblockv1 = 3,
        JournalSuperblockv2 = 4,
        BlockRevocationRecords = 5, // This speeds up recovery by enabling the journal to skip writing blocks that were subsequently rewritten.
    };
}

struct JournalSuperBlock {
    Journal s_header; // Common header identifying this as a superblock.
    uint32_t s_blocksize; // Journal device block size.
    uint32_t s_maxlen; // Total number of blocks in this journal.
    uint32_t s_first; // First block of log information.
    uint32_t s_sequence; // First commit ID expected in log.
    uint32_t s_start; // Block number of the start of log. Contrary to the comments, this field being zero does not imply that the journal is clean!
    uint32_t s_errno; // Error value, as set by jbd2_journal_abort().
    /*
     * The remaining fields are only valid in a v2 superblock.
    */
    uint32_t s_feature_compat; // Compatible feature set.
    uint32_t s_feature_incompat; // Incompatible feature set.
    uint32_t s_feature_ro_compat; // Read-only compatible feature set. There aren't any of these currently.
    uint8_t s_uuid[16]; // 128-bit uuid for journal. This is compared against the copy in the ext4 super block at mount time.
    uint32_t s_nr_users; // Number of file systems sharing this journal.
    uint32_t s_dynsuper; // Location of dynamic super block copy. (Not used?)
    uint32_t s_max_transaction; // Limit of journal blocks per transaction. (Not used?)
    uint32_t s_max_trans_data; // Limit of data blocks per transaction. (Not used?)
    uint8_t s_checksum_type; // Checksum algorithm used for the journal. 
    uint8_t s_padding2[3]; // 
    uint32_t s_num_fc_blocks; // Number of fast commit blocks in the journal. 
    uint32_t s_head; // Block number of the head (first unused block) of the journal, only up-to-date when the journal is empty.
    uint32_t s_padding[40]; // 
    uint32_t s_checksum; // Checksum of the entire superblock, with this field set to zero.
    uint8_t s_users[16 * 48]; // ids of all file systems sharing the log. e2fsprogs/Linux don't allow shared external journals, but I imagine Lustre (or ocfs2?), which use the jbd2 code, might.
} __attribute__((packed));

namespace JournalCompat {
    enum _JournalCompat {
        JBD2_FEATURE_COMPAT_CHECKSUM = 0x1, // Journal maintains checksums on the data blocks. 
    };
}

namespace JournalIncompat {
    enum _JournalIncompat {
        JBD2_FEATURE_INCOMPAT_REVOKE = 0x1, // Journal has block revocation records.
        JBD2_FEATURE_INCOMPAT_64BIT = 0x2, // Journal can deal with 64-bit block numbers.
        JBD2_FEATURE_INCOMPAT_ASYNC_COMMIT = 0x4, // Journal commits asynchronously.
        JBD2_FEATURE_INCOMPAT_CSUM_V2 = 0x8, // This journal uses v2 of the checksum on-disk format. Each journal metadata block gets its own checksum, and the block tags in the descriptor table contain checksums for each of the data blocks in the journal.
        JBD2_FEATURE_INCOMPAT_CSUM_V3 = 0x10, // This journal uses v3 of the checksum on-disk format. This is the same as v2, but the journal block tag size is fixed regardless of the size of block numbers.
        JBD2_FEATURE_INCOMPAT_FAST_COMMIT = 0x20, // Journal has fast commit blocks.
    };
}

namespace JournalChkType {
    enum _JournalChkType {
        CRC32 = 1,
        MD5 = 2,
        SHA1 = 3,
        CRC32C = 4,
    };
}