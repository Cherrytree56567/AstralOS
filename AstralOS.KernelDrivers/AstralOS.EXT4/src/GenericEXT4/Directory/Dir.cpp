#include "../GenericEXT4.h"

void GenericEXT4Device::ParseDirectoryBlock(FsNode**& nodes, uint64_t& count, size_t& capacity, uint64_t block) {
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t LBA = block * sectorsPerBlock;

    for (uint64_t s = 0; s < sectorsPerBlock; s++) {
        if (!pdev->ReadSector(LBA + s, (void*)((uint64_t)bufPhys + s * pdev->SectorSize()))) {
            _ds->Println("Failed to read sector");
            return;
        }
    }

    uint64_t offset = 0;
    while (offset < blockSize) {
        DirectoryEntry2* ent = (DirectoryEntry2*)(bufVirt + offset);

        if (ent->rec_len == 0) {
            offset += ent->rec_len;
            continue;
        }
        if (ent->inode == 0) {
            offset += ent->rec_len;
            continue;
        }

        char nameBuf[256] = {0};

        if (ent->name_len == 0 || ent->name_len >= sizeof(nameBuf)) break;

        memcpy(nameBuf, ent->name, ent->name_len);
        nameBuf[ent->name_len] = '\0';

        if (count >= capacity) {
            size_t newCap = capacity == 0 ? 4 : capacity * 2;
            FsNode** tmp = (FsNode**)_ds->malloc(newCap * sizeof(FsNode*));
            if (!tmp) break;
            if (nodes) {
                memcpy(tmp, nodes, count * sizeof(FsNode*));
                _ds->free(nodes);
            }
            nodes = tmp;
            capacity = newCap;
        }

        FsNode* child = (FsNode*)_ds->malloc(sizeof(FsNode));
        if (!child) break;
        memset(child, 0, sizeof(FsNode));
        child->nodeId = ent->inode;
        switch (ent->file_type) {
            case DirectoryEntryType::EXT4_FT_REG_FIL:
                child->type = FsNodeType::File;
                break;
            case DirectoryEntryType::EXT4_FT_DIR:
                child->type = FsNodeType::Directory;
                break;
            default:
                break;
        }
        child->name = _ds->strdup(nameBuf);

        Inode* childInode = ReadInode(ent->inode);
        if (childInode) {
            child->size = childInode->i_size_lo | ((uint64_t)childInode->i_size_high << 32);
            if (superblock->s_creator_os == CreatorOSIDs::Linux) {
                child->blocks = (uint64_t)childInode->i_osd2.linux2.l_i_blocks_high << 32 | childInode->i_blocks_lo;
            } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
                child->blocks = (uint64_t)childInode->i_osd2.astral2.a_i_blocks_high << 32 | childInode->i_blocks_lo;
            } else {
                child->blocks = childInode->i_blocks_lo;
            }
            
            child->mode = childInode->i_mode;
            child->uid = childInode->i_uid;
            child->gid = childInode->i_gid;

            child->atime = childInode->i_atime;
            child->mtime = childInode->i_mtime;
            child->ctime = childInode->i_crtime;
        }

        nodes[count++] = child;

        offset += ent->rec_len;
    }
}