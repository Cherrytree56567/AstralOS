#include "../GenericEXT4.h"

bool GenericEXT4Device::HasSuperblockBKP(uint32_t group) {
    if (group == 0 || group == 1) return true;
    if (!(superblock->s_feature_ro_compat & ROFeatures::RO_COMPAT_SPARSE_SUPER)) return true;

    if (isPower(group, 3)) return true;
    if (isPower(group, 5)) return true;
    if (isPower(group, 7)) return true;

    return false;
}

/*
 * Updating the superblock is pretty easy
 * since all we need to do is read, copy
 * and then write the superblock. We must
 * also update the backup superblocks,
 * which is what the above function is for.
 * 
 * Updating the checksum is also pretty
 * easy, all we need to do is use ~0 as a
 * seed and generate the FilesystemUUID
 * crc32c for the CheckUUID. This UUID is
 * what we use for the superblock, group
 * desc and Inodes.
 * 
 * Then we zero the checksum and generate
 * a crc32c checksum based on the CheckUUID
 * and superblock.
*/
void GenericEXT4Device::UpdateSuperblock() {
    if (superblock->s_checksum_type == 1) {
        superblock->s_checksum = crc32c_sw(~0, superblock->s_uuid, 16);
        superblock->s_checksum = crc32c_sw(superblock->s_checksum, superblock, offsetof(EXT4_Superblock, s_checksum));
    }
    uint32_t sectorSize = pdev->SectorSize();

    uint64_t superblockSize = 1024;
    uint64_t SuperblockOffset = 1024;
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;

    auto writeSuperblock = [&](uint64_t blockNum) {
        uint64_t LBA = (blockNum * blockSize) / sectorSize;
        uint64_t Offset = (blockNum * blockSize) % sectorSize;
        uint64_t sectorsNeeded = (Offset + superblockSize + sectorSize - 1) / sectorSize;

        void* bufPhys = _ds->RequestPage();
        uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
        _ds->MapMemory((void*)bufVirt, bufPhys, false);
        memset((void*)bufVirt, 0, 4096);

        for (uint64_t i = 0; i < sectorsNeeded; i++) {
            if (!pdev->ReadSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
                _ds->Println("Failed to read superblock backup");
                return;
            }
        }

        EXT4_Superblock* sup = (EXT4_Superblock*)bufVirt + Offset;

        memcpy((uint8_t*)bufVirt + Offset, superblock, superblockSize);

        for (uint64_t i = 0; i < sectorsNeeded; i++) {
            if (!pdev->WriteSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
                _ds->Print("Failed to write superblock backup: ");
                _ds->Println(to_hstridng(LBA + i));
                return;
            }
        }
    };

    writeSuperblock(SuperblockOffset / blockSize);

    if (superblock->s_feature_ro_compat & ROFeatures::RO_COMPAT_SPARSE_SUPER != 0) {
        uint64_t blocks = superblock->s_blocks_count_lo;
        blocks |= ((uint64_t)superblock->s_blocks_count_hi) << 32;

        uint32_t totalBlockGroups = (blocks + superblock->s_blocks_per_group - 1) / superblock->s_blocks_per_group;
        for (uint32_t bg = 0; bg < totalBlockGroups; bg++) {
            if (bg == 0 || bg == 1 || bg % 3 == 0 || bg % 5 == 0 || bg % 7 == 0) {
                uint64_t backupBlock = bg * superblock->s_blocks_per_group + superblock->s_first_data_block;
                if (backupBlock != SuperblockOffset / blockSize) {
                    writeSuperblock(backupBlock);
                }
            }
        }
    } else {
        _ds->Println("Filesystem does not use sparse superblocks, no backups written");
    }
}