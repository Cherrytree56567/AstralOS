#include "GenericEXT4.h"

FsNode** GenericEXT4Device::ParseDirectoryBlock(uint8_t* block, uint64_t& FSNcount) {
    uint32_t off = 0;
    uint32_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t count = 0;

    while (off < blockSize) {
        DirectoryEntry* ent = (DirectoryEntry*)(block + off);

        if (ent->TotalSize == 0) break;
        if (off + ent->TotalSize > blockSize) break;

        if (ent->Inode != 0 && ent->NameLen != 0) {
            count++;
        }

        off += ent->TotalSize;
    }

    FsNode** fsN = (FsNode**)_ds->malloc(count * sizeof(FsNode*));
    uint64_t i = 0;
    off = 0;

    while (off < blockSize) {
        DirectoryEntry* ent = (DirectoryEntry*)(block + off);

        if (ent->TotalSize == 0) break;
        if (off + ent->TotalSize > blockSize) break;

        if (ent->Inode != 0 && ent->NameLen != 0) {
            char nameBuf[256] = {0};

            if (ent->NameLen == 0 || ent->NameLen >= sizeof(nameBuf)) break;

            memcpy(nameBuf, ent->Name, ent->NameLen);
            nameBuf[ent->NameLen] = '\0';

            FsNode* child = (FsNode*)_ds->malloc(sizeof(FsNode));
            if (!child) break;
            memset(child, 0, sizeof(FsNode));
            child->nodeId = ent->Inode;
            switch (ent->Type) {
                case InodeTypeEnum::File:
                    child->type = FsNodeType::File;
                    break;
                case InodeTypeEnum::Directory:
                    child->type = FsNodeType::Directory;
                    break;
                default:
                    break;
            }
            child->name = _ds->strdup(nameBuf);

            Inode* childInode = ReadInode(ent->Inode);
            if (childInode) {
                child->size = childInode->LowSize | ((uint64_t)childInode->HighSize << 32);
                child->blocks = childInode->LowBlocks;

                child->mode = childInode->mode;
                child->uid = childInode->UserID;
                child->gid = childInode->GroupID;

                child->atime = childInode->LastAccess;
                child->mtime = childInode->LastModification;
                child->ctime = childInode->FileCreation;
            }

            fsN[i++] = child;
        }

        off += ent->TotalSize;
    }
    FSNcount = count;
    return fsN;
}