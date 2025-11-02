#include "GenericEXT4.h"
#include "../global.h"
#include <new>

const char* to_hstridng(uint64_t value) {
    static char buffer[19];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    do {
        uint8_t nibble = value & 0xF;
        *--ptr = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    } while (value > 0);

    *--ptr = 'x';
    *--ptr = '0';

    return ptr;
}

bool GenericEXT4::Supports(const DeviceKey& devKey) {
    if (devKey.bars[2] == 22) {
        uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
        BaseDriver* bsdrv = (BaseDriver*)dev;
        if (bsdrv->GetDriverType() == Partition) {
            return true;
        }
    }
    return false;
}

FilesystemDevice* GenericEXT4::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericEXT4Device));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic EXT4 Device");
        return nullptr;
    }

    GenericEXT4Device* device = new(mem) GenericEXT4Device();
    return device;
}

void* memset(void* dest, uint8_t value, size_t num) {
    uint8_t* ptr = (uint8_t*)dest;
    for (size_t i = 0; i < num; i++) {
        ptr[i] = value;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];

    return dest;
}

void GenericEXT4Device::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
    BaseDriver* bsdrv = (BaseDriver*)dev;
    pdev = (PartitionDevice*)bsdrv;

    _ds->Println("Initialized Generic EXT4 Driver");
}

/*
 * First we need to check if the SectorCount is over 0x3
 * and if the SectorSize isn't 0. Then we get the LBA and
 * the LBASize. Then we can efficiently read the Super
 * Block and check the signature.
*/
bool GenericEXT4Device::Support() {
    if (pdev->SectorCount() < 0x3 || pdev->SectorSize() == 0) {
        return false;
    }
    /*
     * Here we can use LBA for LBA and LBASize
    */
    uint32_t LBA = 1024 / pdev->SectorSize();
    uint32_t offset = 1024 % pdev->SectorSize();
    if (LBA < 1) {
        LBA = 1;
    }

    uint32_t LBASize = (1024 + pdev->SectorSize() - 1) / pdev->SectorSize();

    if (pdev->SectorCount() == 0) return false;

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t offsetI = 0;
    for (size_t i = 0; i < LBASize; i++) {
        if (!pdev->ReadSector(LBA + i, (void*)(bufPhys + offsetI))) {
            _ds->Println("Failed To Read Sector : EXT4");
            return false;
        }
        offsetI += pdev->SectorSize();
    }
    superblock = (EXT4_Superblock*)(bufVirt + offset);

    _ds->Print("FS Signature: ");
    _ds->Println(to_hstridng(superblock->MagicSignature));

    if (superblock->MagicSignature == 0xEF53) {
        if (superblock->RequiredFeatures & RequiredFeatures::FileExtent) {
            return true;
        }
    }
    return false;
}

FsNode* GenericEXT4Device::Mount() {
    if (!Support()) {
        return NULL;
    }

    if (isMounted) {
        return NULL;
    }

    FsNode* node = (FsNode*)_ds->malloc(sizeof(FsNode));
    if (!node) return nullptr;
    memset(node, 0, sizeof(FsNode));

    /*
     * The root inode number is always 2
    */
    uint64_t InodeBlockGroup = (2 - 1) / superblock->InodesPerBlockGroup;
    uint64_t InodeIndex = (2 - 1) % superblock->InodesPerBlockGroup;

    uint64_t BlockSize = 1024ull << superblock->BlockSize;

    uint64_t SectorsBlock = BlockSize / pdev->SectorSize();

    uint64_t BlockGroupDescLBA;
    if (BlockSize == 1024) {
        BlockGroupDescLBA = 2 * SectorsBlock;
    } else {
        BlockGroupDescLBA = 1 * SectorsBlock;
    }

    uint64_t GroupDescSize = (superblock->GroupDescriptorBytes) ? superblock->GroupDescriptorBytes : 32;

    uint64_t BlockGroupDescOff = InodeBlockGroup * GroupDescSize;

    uint64_t DescLBA = BlockGroupDescLBA + (BlockGroupDescOff / pdev->SectorSize());
    uint64_t DescOff = BlockGroupDescOff % pdev->SectorSize();

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t sectors_in_block = BlockSize / pdev->SectorSize();
    for (uint64_t i = 0; i < sectors_in_block; i++) {
        if (!pdev->ReadSector(DescLBA + i, (void*)(bufPhys + (i * pdev->SectorSize())))) {
            _ds->Println("Failed to read block group descriptor");
            return nullptr;
        }
    }

    BlockGroupDescriptor* GroupDesc = (BlockGroupDescriptor*)(bufVirt + DescOff);
    uint64_t InodeTableBlock = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | (uint64_t)GroupDesc->LowAddrInodeTable;
    uint64_t InodeTableLBA = InodeTableBlock * (BlockSize / pdev->SectorSize());
    uint64_t InodeOffset = InodeIndex * superblock->InodeSize;
    uint64_t InodeSize = superblock->InodeSize;

    uint64_t InodeSectors = (InodeSize + pdev->SectorSize() - 1) / pdev->SectorSize();
    for (uint64_t i = 0; i < InodeSectors; i++) {
        uint64_t LBA = InodeTableLBA + i;
        uint64_t OffsetBuf = i * pdev->SectorSize();
        if (!pdev->ReadSector(LBA, (void*)(bufPhys + OffsetBuf))) {
            _ds->Println("Failed to read root inode");
            return nullptr;
        }
    }
    Inode root_inode = *(Inode*)(bufVirt + (InodeOffset % pdev->SectorSize()));

    node->nodeId = 2;
    uint16_t raw = *(uint16_t*)&root_inode.meta;
    _ds->Print(to_hstridng(raw));

    if (root_inode.meta.Type == 0x4) {
        node->type = FsNodeType::Directory;
    } else if (root_inode.meta.Type == 0x8) {
        node->type = FsNodeType::File;
    } else if (root_inode.meta.Type == 0xA) {
        node->type = FsNodeType::Symlink;
    } else {
        node->type = FsNodeType::Unknown;
    }

    node->name = _ds->strdup("/");
    node->size = (uint64_t)root_inode.UpperFileSize << 32 | root_inode.LowerSize;
    node->blocks = root_inode.DiskSectorCount;

    /*
     * These lines (200-211) are GPT Automated Code.
    */
    uint16_t perms = 0;
    if (root_inode.meta.userRead) perms |= 0b100000000;
    if (root_inode.meta.userWrite) perms |= 0b010000000;
    if (root_inode.meta.userExec) perms |= 0b001000000;

    if (root_inode.meta.groupRead) perms |= 0b000100000;
    if (root_inode.meta.groupWrite) perms |= 0b000010000;
    if (root_inode.meta.groupExec) perms |= 0b000001000;

    if (root_inode.meta.otherRead) perms |= 0b000000100;
    if (root_inode.meta.otherWrite) perms |= 0b000000010;
    if (root_inode.meta.otherExec) perms |= 0b000000001;

    node->mode = perms;
    node->uid = root_inode.UserID;
    node->gid = root_inode.GroupID;

    node->atime = root_inode.LastAccess;
    node->mtime = root_inode.LastModification;
    node->ctime = root_inode.Creation;

    pdev->SetMount(EXT4MountID);
    pdev->SetMountNode(node);

    isMounted = true;

    _ds->Println("Mounted EXT4 Filesystem!");

    return node;
}

Inode* GenericEXT4Device::ReadInode(uint64_t node) {
    uint64_t InodeBlockGroup = (node - 1) / superblock->InodesPerBlockGroup;
    uint64_t InodeIndex = (node - 1) % superblock->InodesPerBlockGroup;

    uint64_t BlockSize = 1024ull << superblock->BlockSize;

    uint64_t SectorsBlock = BlockSize / pdev->SectorSize();

    uint64_t BlockGroupDescLBA;
    if (BlockSize == 1024) {
        BlockGroupDescLBA = 2 * SectorsBlock;
    } else {
        BlockGroupDescLBA = 1 * SectorsBlock;
    }

    uint64_t GroupDescSize = (superblock->GroupDescriptorBytes) ? superblock->GroupDescriptorBytes : 32;

    uint64_t BlockGroupDescOff = InodeBlockGroup * GroupDescSize;

    uint64_t DescLBA = BlockGroupDescLBA + (BlockGroupDescOff / pdev->SectorSize());
    uint64_t DescOff = BlockGroupDescOff % pdev->SectorSize();

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t sectors_in_block = BlockSize / pdev->SectorSize();
    for (uint64_t i = 0; i < sectors_in_block; i++) {
        if (!pdev->ReadSector(DescLBA + i, (void*)(bufPhys + (i * pdev->SectorSize())))) {
            _ds->Println("Failed to read block group descriptor");
            return nullptr;
        }
    }

    BlockGroupDescriptor* GroupDesc = (BlockGroupDescriptor*)(bufVirt + DescOff);

    uint64_t InodeTableBlock = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | (uint64_t)GroupDesc->LowAddrInodeTable;
    uint64_t InodeTableLBA = InodeTableBlock * (BlockSize / pdev->SectorSize());
    uint64_t InodeOffset = InodeIndex * superblock->InodeSize;
    uint64_t InodeSize = superblock->InodeSize;

    uint64_t InodeSectors = (InodeSize + pdev->SectorSize() - 1) / pdev->SectorSize();
    for (uint64_t i = 0; i < InodeSectors; i++) {
        uint64_t LBA = InodeTableLBA + i;
        uint64_t OffsetBuf = i * pdev->SectorSize();
        if (!pdev->ReadSector(LBA, (void*)(bufPhys + OffsetBuf))) {
            _ds->Println("Failed to read root inode");
            return nullptr;
        }
    }
    return (Inode*)(bufVirt + (InodeOffset % pdev->SectorSize()));
}

bool GenericEXT4Device::Unmount() {
    if (!isMounted) {
        return false;
    }
    if (pdev->SetMount(0xA574A105)) { // Unmounted Code
        return false;
    }

    FsNode fsn;
    if (pdev->SetMountNode(&fsn)) {
        return false;
    }
    return true;
}

FsNode** GenericEXT4Device::ListDir(FsNode* node, size_t* outCount) {
    if (!isMounted) {
        return nullptr;
    }
    if (node->type != FsNodeType::Directory) {
        return nullptr;
    }

    Inode dir_inode = *ReadInode(node->nodeId);

    FsNode** nodes = nullptr;
    size_t count = 0;
    size_t capacity = 0;

    uint64_t blockSize = 1024ull << superblock->BlockSize;
    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

    for (uint64_t i = 0; i < 15; i++) {
        if (dir_inode.DBP[i] == 0) continue;
        memset((void*)bufVirt, 0, 4096);
        uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

        for (uint64_t s = 0; s < sectorsPerBlock; s++) {
            uint64_t lba = dir_inode.DBP[i] * sectorsPerBlock + s;
            if (!pdev->ReadSector(lba, (void*)((uint64_t)bufPhys + s * pdev->SectorSize()))) {
                _ds->Println("Failed to read sector");
                return nullptr;
            }
        }

        uint64_t offset = 0;
        while (offset < blockSize) {
            DirectoryEntry* entry = (DirectoryEntry*)(bufVirt + offset);
            if (entry->Inode == 0) break;
            if (entry->TotalSize < sizeof(DirectoryEntry)) break;
            if (entry->Type != 0x2) break;

            char nameBuf[256] = {0};

            if (entry->NameLen == 0 || entry->NameLen >= sizeof(nameBuf)) break;

            memcpy(nameBuf, entry->Name, entry->NameLen);
            nameBuf[entry->NameLen] = '\0';

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
            child->nodeId = entry->Inode;
            child->type = FsNodeType::Directory;
            child->name = _ds->strdup(nameBuf);

            Inode* childInode = ReadInode(entry->Inode);
            if (childInode) {
                child->size = childInode->LowerSize | ((uint64_t)childInode->UpperFileSize << 32);
                child->blocks = childInode->DiskSectorCount;

                uint16_t perms = 0;
                if (childInode->meta.userRead) perms |= 0b100000000;
                if (childInode->meta.userWrite) perms |= 0b010000000;
                if (childInode->meta.userExec) perms |= 0b001000000;

                if (childInode->meta.groupRead) perms |= 0b000100000;
                if (childInode->meta.groupWrite) perms |= 0b000010000;
                if (childInode->meta.groupExec) perms |= 0b000001000;

                if (childInode->meta.otherRead) perms |= 0b000000100;
                if (childInode->meta.otherWrite) perms |= 0b000000010;
                if (childInode->meta.otherExec) perms |= 0b000000001;

                child->mode = perms;
                child->uid  = childInode->UserID;
                child->gid  = childInode->GroupID;

                child->atime = childInode->LastAccess;
                child->mtime = childInode->LastModification;
                child->ctime = childInode->Creation;
            }

            nodes[count++] = child;

            offset += entry->TotalSize;
        }
    }

    *outCount = count;
    return nodes;
}

FsNode* GenericEXT4Device::FindDir(FsNode* node, const char* name) {
    
}

FsNode* GenericEXT4Device::CreateDir(FsNode* parent, const char* name) {
    
}

bool GenericEXT4Device::Remove(FsNode* node) {
    
}

File* GenericEXT4Device::Open(FsNode* node, uint32_t flags) {
    
}

bool GenericEXT4Device::Close(File* file) {
    
}

int64_t GenericEXT4Device::Read(File* file, void* buffer, uint64_t size) {
    
}

int64_t GenericEXT4Device::Write(File* file, void* buffer, uint64_t size) {
    
}

bool GenericEXT4Device::Stat(FsNode* node, FsNode* out) {
    
}

bool GenericEXT4Device::Chmod(FsNode* node, uint32_t mode) {
    
}

bool GenericEXT4Device::Chown(FsNode* node, uint32_t uid, uint32_t gid) {
    
}

bool GenericEXT4Device::Utimes(FsNode* node, uint64_t atime, uint64_t mtime, uint64_t ctime) {
    
}

uint8_t GenericEXT4Device::GetClass() {
    return 0x0;
}

uint8_t GenericEXT4Device::GetSubClass() {
    return 0x0;
}

uint8_t GenericEXT4Device::GetProgIF() {
    return 0x0;
}

const char* GenericEXT4Device::name() const {
    return "";
}

/*
 * Make sure you return somthing
 * otherwise you will get weird
 * errors, like code that ends up
 * at 0x8 in mem.
*/
const char* GenericEXT4Device::DriverName() const {
    return _ds->strdup("Generic EXT4 Driver");
}

PartitionDevice* GenericEXT4Device::GetParentLayer() {
    return pdev;
}