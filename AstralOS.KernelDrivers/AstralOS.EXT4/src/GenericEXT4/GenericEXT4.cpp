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

constexpr uint64_t ceil(uint64_t a, uint64_t b) {
    return (a + b - 1) / b;
}

int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
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

    uint64_t totalBlocks = ((uint64_t)superblock->HighBlocks << 32) | superblock->TotalBlocks;

    uint32_t BlockGroupCount = (totalBlocks + superblock->BlocksPerBlockGroup - 1) / superblock->BlocksPerBlockGroup;

    GroupDescs = (BlockGroupDescriptor**)_ds->malloc(BlockGroupCount * sizeof(BlockGroupDescriptor*));
    if (!GroupDescs) return nullptr;
    memset(GroupDescs, 0, BlockGroupCount * sizeof(BlockGroupDescriptor*));

    for (uint32_t group = 0; group < BlockGroupCount; group++) {
        BlockGroupDescriptor* BlockGroupDesc = ReadGroupDesc(group);

        if (!BlockGroupDesc) {
            _ds->Print("Failed to read group descriptor");
        }

        GroupDescs[group] = BlockGroupDesc;
    }

    /*
     * The root inode number is always 2
    */
    Inode* inode = ReadInode(2);
    if (!inode) {
        _ds->Println("Failed to read root inode!");
        return nullptr;
    }
    Inode root_inode = *inode;

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

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    BlockGroupDescriptor* GroupDesc = GroupDescs[InodeBlockGroup];

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
            if (entry->Type != 0x2 && entry->Type != 0x1) break;

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
            switch (entry->Type) {
                case 0x1:
                    child->type = FsNodeType::File;
                    break;
                case 0x2:
                    child->type = FsNodeType::Directory;
                    break;
                default:
                    break;
            }
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

size_t strlen(const char *str) {
    size_t count = 0;
    while (str[count] != '\0') {
        count++;
    }
    return count;
}

/*
size_t strlen(const char *str) {
    size_t count = 0;
    while (str[count] != '\0') {
        count++;
    }
    return count;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c)
            return (char*)s;
        s++;
    }

    if (c == '\0')
        return (char*)s;

    return NULL;
}

char* strtok(char* str, const char* delim) {
    static char* next = NULL;

    if (str)
        next = str;

    if (!next)
        return NULL;
        
    char* start = next;
    while (*start && strchr(delim, *start))
        start++;

    if (*start == '\0') {
        next = NULL;
        return NULL;
    }

    char* end = start;
    while (*end && !strchr(delim, *end))
        end++;

    if (*end) {
        *end = '\0';
        next = end + 1;
    } else {
        next = NULL;
    }

    return start;
}

/*
 * Found on Stack Overflow:
 * https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
/
char** str_split(DriverServices* _ds, char* a_str, const char a_delim, size_t* size) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    / Count how many elements will be extracted. /
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    / Add space for trailing token. /
    count += last_comma < (a_str + strlen(a_str) - 1);

    / Add space for terminating null string so caller
       knows where the list of returned strings ends. /
    count++;

    result = (char**)_ds->malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = _ds->strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }
    size = &count;

    return result;
}
*/

/*
 * To Find a File through a Dir you can just
 * use `ListDir` to get all the files in the
 * node and recursively look through subdirs
 * to find the file and return the FsNode*.
*/
FsNode* GenericEXT4Device::FindDir(FsNode* node, const char* name) {
    size_t count = 0;
    FsNode** contents = ListDir(node, &count);

    for (int x = 0; x < count; x++) {
        if (strcmp(contents[x]->name, name) == 0) {
            return contents[x];
        }

        if (strcmp(contents[x]->name, ".") == 0) continue;
        if (strcmp(contents[x]->name, "..") == 0) continue;

        if (contents[x]->type == FsNodeType::Directory) {
            FsNode* fsn = FindDir(contents[x], name);
            if (fsn) return fsn;
        }
    }
    _ds->free(contents);
    return NULL;
}

/*
 * Because we implemented an AllocInode,
 * everything else is pretty easy.
 * 
 * First we need to Allocate an Inode
 * and fill in the Inode.
 * Then we can allocate a block and create
 * two dir entries which are `.` and `..`,
 * then we can finally write it to disk.
 * 
 * tbh writing this func was pretty hard so
 * I needed to ask GPT. It did help me out
 * but most of the code it gave me was the
 * code I wrote in the first place.
 * 
 * 1. Alloc an Inode
 * 2. Alloc a Block
 * 3. Fill in the Inode
 * 4. Create 2 Dir Entries:
 *  - `.` - Points to curr Dir
 *  - `..` - Points to Parent Dir
 * 5. Write the Dir Entries
 * 6.1. Read The Inode LBA
 * 6.2. Write the New Inode to the LBA
 * 7. Read the Parent Inode
 * 8. Modify the:
 *  - Last Mod Time
 *  - Access Time
 *  - Hard Links Count
 *  - Size
 *  - Disk Sector Count
 * 9. Read and write to Disk
 * 10. Write the Dir Entry to the Parent DIr
 * 11. Create and Pass the FsNode*
*/
FsNode* GenericEXT4Device::CreateDir(FsNode* parent, const char* name) {
    if (!parent) return NULL;
    if (parent->type != FsNodeType::Directory) return NULL;

    uint32_t InodeNum = AllocateInode();
    if (!InodeNum) {
        _ds->Println("Failed to allocate inode");
        return nullptr;
    }

    uint64_t blockSize = 1024ull << superblock->BlockSize;

    Inode newInode;
    memset(&newInode, 0, sizeof(Inode));
    newInode.meta.Type = 0x4;
    newInode.meta.userRead = 1;
    newInode.meta.userWrite = 1;
    newInode.meta.userExec = 1;

    newInode.meta.groupRead = 1;
    newInode.meta.groupWrite = 0;
    newInode.meta.groupExec = 1;

    newInode.meta.otherRead = 1;
    newInode.meta.otherWrite = 0;
    newInode.meta.otherExec = 1;
    newInode.UserID = 0;
    newInode.GroupID = 0;

    uint32_t now = 0x6943C554; // TODO: currentTime();
    newInode.Creation = now;
    newInode.LastAccess = now;
    newInode.LastModification = now;
    newInode.Deletion = 0;

    newInode.HardLinksCount = 2;

    newInode.LowerSize = blockSize; 
    newInode.UpperFileSize = 0;
    newInode.DiskSectorCount = blockSize / 512;

    uint32_t dirBlock = AllocateBlock();
    if (dirBlock == 0) {
        _ds->Println("Failed to allocate block");
        return nullptr;
    }
    newInode.DBP[0] = dirBlock;

    newInode.Flags = 0;
    OSVal2 osv;
    osv.EasterEgg = 0xA574A105;
    osv.EasterEggChristmas = 0xC671573A5;
    osv.FragSize = 1024ull << superblock->FragmentSize;
    osv.FragNum = 0; // Not Used
    osv.HighGroupID = 0;
    osv.HighUserID = 0;
    newInode.osval2 = osv;
    newInode.Generation = 0;
    newInode.ExtendedAttributeBlock = 0;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    DirectoryEntry* curr = (DirectoryEntry*)bufVirt;
    curr->Inode = InodeNum;
    curr->TotalSize = (uint16_t)((8 + 1 + 3) & ~3);
    curr->NameLen = 1;
    curr->Type = 2;
    curr->Name[0] = '.';

    DirectoryEntry* par = (DirectoryEntry*)((uint8_t*)bufVirt + curr->TotalSize);
    par->Inode = parent->nodeId;
    par->TotalSize = 4096 - curr->TotalSize;
    par->NameLen = 2;
    par->Type = 2;
    par->Name[0] = '.';
    par->Name[1] = '.';

    uint64_t blockLBA = dirBlock * (1024ull << superblock->BlockSize) / pdev->SectorSize();
    uint64_t sectorsInBlock = (1024ull << superblock->BlockSize) / pdev->SectorSize();
    for (uint64_t i = 0; i < sectorsInBlock; i++) {
        if (!pdev->WriteSector(blockLBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
            _ds->Println("Failed to write directory block");
            return nullptr;
        }
    }

    uint32_t inodeIndex = InodeNum - 1;
    uint32_t inodesPerGroup = superblock->InodesPerBlockGroup;

    uint32_t group = inodeIndex / inodesPerGroup;
    uint32_t indexInGroup = inodeIndex % inodesPerGroup;

    BlockGroupDescriptor* GroupDesc = GroupDescs[group];
    uint64_t inodeTableBlock = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | GroupDesc->LowAddrInodeTable;

    uint32_t inodeSize = superblock->InodeSize;
    uint64_t byteOff = indexInGroup * inodeSize;

    uint64_t blockOffset = byteOff / blockSize;
    uint64_t offsetInBlock = byteOff % blockSize;

    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / sectorSize;

    uint64_t BlockLBA = (inodeTableBlock + blockOffset) * sectorsPerBlock;

    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(BlockLBA + i, (void*)((uint8_t*)bufPhys + i * sectorSize))) {
            _ds->Println("Block bitmap read failed");
            return nullptr;
        }
    }

    memcpy((uint8_t*)bufVirt + offsetInBlock, &newInode, inodeSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->WriteSector(BlockLBA + i, (void*)((uint8_t*)bufPhys + i * sectorSize))) {
            _ds->Println("Block bitmap write failed");
            return nullptr;
        }
    }

    Inode parentInode = *ReadInode(parent->nodeId);

    parentInode.HardLinksCount++;
    parentInode.LastModification = now;
    parentInode.LastAccess = now;
    parentInode.LowerSize += blockSize;
    parentInode.DiskSectorCount += blockSize / pdev->SectorSize();
    uint32_t newBlockNum = AllocateBlock();

    int nextFreeIndex = -1;
    for (int i = 0; i < 12; i++) {
        if (parentInode.DBP[i] == 0) {
            nextFreeIndex = i;
            break;
        }
    }

    if (nextFreeIndex == -1) {
        _ds->Println("Parent has no free DBP slots!");
        return nullptr;
    }

    parentInode.DBP[nextFreeIndex] = newBlockNum;

    inodeIndex = parent->nodeId - 1;

    group = inodeIndex / inodesPerGroup;
    indexInGroup = inodeIndex % inodesPerGroup;

    GroupDesc = GroupDescs[group];
    inodeTableBlock = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | GroupDesc->LowAddrInodeTable;

    uint64_t byteOffset = indexInGroup * superblock->InodeSize;

    blockOffset = byteOffset / blockSize;
    offsetInBlock = byteOffset % blockSize;

    blockLBA = (inodeTableBlock + blockOffset) * sectorsPerBlock;

    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        pdev->ReadSector(blockLBA + i, (void*)((uint8_t*)bufPhys + i * sectorSize));
    }

    memcpy((uint8_t*)bufVirt + offsetInBlock, &parentInode, superblock->InodeSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        pdev->WriteSector(blockLBA + i, (void*)((uint8_t*)bufPhys + i * sectorSize));
    }

    uint16_t newEntrySize = (uint16_t)((8 + ((uint8_t)strlen(name)) + 3) & ~3);
    bool Inserted = false;

    for (int i = 0; i < 12; i++) {
        uint32_t blockNum = parentInode.DBP[i];
        if (blockNum == 0) continue;

        memset((void*)bufVirt, 0, 4096);

        for (uint64_t x = 0; x < sectorsPerBlock; x++) {
            uint64_t lba = blockNum * sectorsPerBlock;
            if (!pdev->ReadSector(lba + x, (void*)((uint8_t*)bufPhys + x * sectorSize))) {
                _ds->Print("Failed to Read Block!");
            }
        }

        uint64_t offset = 0;
        while (offset < blockSize) {
            DirectoryEntry* e = (DirectoryEntry*)((uint8_t*)bufVirt + offset);
            uint16_t actualSize = (uint16_t)((8 + (e->NameLen) + 3) & ~3);
            uint16_t freeSpace = e->TotalSize - actualSize;

            if (freeSpace >= newEntrySize) {
                DirectoryEntry* newEnt = (DirectoryEntry*)((uint8_t*)e + actualSize);
                newEnt->Inode = InodeNum;
                newEnt->NameLen = strlen(name);
                newEnt->Type = 0x2;
                memcpy(newEnt->Name, name, newEnt->NameLen);
                newEnt->TotalSize = e->TotalSize - actualSize;
                e->TotalSize = actualSize;
                uint64_t lba = blockNum * sectorsPerBlock;
                for (uint64_t x = 0; x < sectorsPerBlock; x++) {
                    if (!pdev->WriteSector(lba + x, (uint8_t*)bufPhys + x * sectorSize)) {
                        _ds->Println("Failed to write parent directory block");
                        return nullptr;
                    }
                }
                Inserted = true;
                break;
            }

            offset += e->TotalSize;
        }
    }

    if (!Inserted) {
        void* BlockBuf = _ds->RequestPage();
        uint64_t BlockBufVirt = (uint64_t)BlockBuf + 0xFFFFFFFF00000000;
        uint64_t BlockBufPhys = (uint64_t)BlockBuf;
        _ds->MapMemory((void*)BlockBufVirt, BlockBuf, false);
        memset((void*)BlockBufVirt, 0, blockSize);

        DirectoryEntry* e = (DirectoryEntry*)BlockBuf;
        e->Inode = InodeNum;
        e->NameLen = strlen(name);
        e->Type = 0x2;
        memcpy(e->Name, name, e->NameLen);
        e->TotalSize = blockSize;

        uint64_t lba = newBlockNum * sectorsPerBlock;

        for (uint64_t x = 0; x < sectorsPerBlock; x++) {
            if (!pdev->WriteSector(lba + x, (void*)((uint8_t*)BlockBufPhys + x * sectorSize))) {
                _ds->Print("Failed to Read Block!");
            }
        }
    }

    FsNode fsN;
    fsN.type = FsNodeType::Directory;
    fsN.name = _ds->strdup(name);
    fsN.nodeId = InodeNum;

    fsN.size = ((uint64_t)newInode.UpperFileSize << 32) | newInode.LowerSize;
    fsN.blocks = newInode.DiskSectorCount;

    uint32_t mode = 0;
    if (newInode.meta.userRead)  mode |= 0400;
    if (newInode.meta.userWrite) mode |= 0200;
    if (newInode.meta.userExec)  mode |= 0100;
    if (newInode.meta.groupRead)  mode |= 0040;
    if (newInode.meta.groupWrite) mode |= 0020;
    if (newInode.meta.groupExec)  mode |= 0010;
    if (newInode.meta.otherRead)  mode |= 0004;
    if (newInode.meta.otherWrite) mode |= 0002;
    if (newInode.meta.otherExec)  mode |= 0001;
    if (newInode.meta.setUserID)  mode |= 04000;
    if (newInode.meta.setGroupID) mode |= 02000;
    if (newInode.meta.stickyBit)  mode |= 01000;

    switch (newInode.meta.Type) {
        case 0x4: mode |= 0040000; break;
        default: break;
    }

    fsN.mode = mode;
    fsN.uid = 0;
    fsN.gid = 0;

    fsN.atime = newInode.LastAccess;
    fsN.mtime = newInode.LastModification;
    fsN.ctime = newInode.Creation;

    return &fsN;
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

/*
 * We can use this to read all Group Descs
 * at the Mount func to easily be able to
 * use them later on.
 * 
 * To read a Group Descriptor we need to first 
 * get the block Size and the sectors per
 * block. Then we can get the First GDT Block
 * and LBA and the desc Size, Offset, LBA and
 * Byte Offset. Then we can read the block 
 * group descriptor.
*/
BlockGroupDescriptor* GenericEXT4Device::ReadGroupDesc(uint32_t group) {
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    uint64_t FirstGDTBlock = (blockSize == 1024) ? 2 : 1;
    uint64_t FirstGDTLBA = FirstGDTBlock * sectorsPerBlock;

    uint64_t descSize = superblock->GroupDescriptorBytes ? superblock->GroupDescriptorBytes : 32;
    uint64_t descByteOffset = group * descSize;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t descLBA = FirstGDTLBA + (descByteOffset / sectorSize);
    uint64_t descOff = descByteOffset % sectorSize;

    uint64_t bytesNeeded = descOff + descSize;
    uint64_t sectorsNeeded = (bytesNeeded + sectorSize - 1) / sectorSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;

    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectorsNeeded; i++) {
        if (!pdev->ReadSector(descLBA + i, (void*)((uint8_t*)bufPhys + i * sectorSize))) {
            _ds->Print("GDT read failed");
        }
    }
    return (BlockGroupDescriptor*)((uint8_t*)bufVirt + descOff);
}

/*
 * To Allocate an Inode we need to 
 * first look through all block
 * group descriptors and check if 
 * they have a free Inode. 
 * 
 * Then we can get and Inode Bitmap 
 * Block and size.
 * 
 * Then we can just read the Inode
 * Bitmap Block and mark it used
 * and finally return the Inode
 * num.
*/
uint32_t GenericEXT4Device::AllocateInode() {
    uint32_t groupCount = (superblock->TotalInodes + superblock->InodesPerBlockGroup - 1) / superblock->InodesPerBlockGroup;

    for (uint32_t group = 0; group < groupCount; group++) {
        uint32_t freeInodes = ((uint32_t)GroupDescs[group]->HighFreeInodes << 16) | (uint32_t)GroupDescs[group]->LowFreeInodes;
        if (freeInodes == 0) continue;

        _ds->Print("Free Inodes Found: ");
        _ds->Println(to_hstridng(freeInodes));

        uint64_t inodeBitmapBlock = ((uint64_t)GroupDescs[group]->HighAddrInodeBitmap << 32) | GroupDescs[group]->LowAddrInodeBitmap;
        uint64_t inodeBitmapSize = (superblock->InodesPerBlockGroup + 7) / 8; // bytes
        uint64_t sectorSize = pdev->SectorSize();
        uint64_t sectorsNeeded = (inodeBitmapSize + sectorSize - 1) / sectorSize;

        void* bufPhys = _ds->RequestPage();
        uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
        _ds->MapMemory((void*)bufVirt, bufPhys, false);
        memset((void*)bufVirt, 0, 4096);

        for (uint64_t i = 0; i < sectorsNeeded; i++) {
            uint64_t lba = inodeBitmapBlock * (1024ull << superblock->BlockSize) / sectorSize + i;
            if (!pdev->ReadSector(lba, (void*)((uint8_t*)bufVirt + i * sectorSize))) {
                _ds->Print("Inode bitmap read failed");
                return 0;
            }
        }

        uint8_t* bitmap = (uint8_t*)bufVirt;
        bool found = false;
        uint32_t inodeIndex = superblock->FirstInode;

        for (; inodeIndex < superblock->InodesPerBlockGroup; inodeIndex++) {
            if (!(bitmap[inodeIndex / 8] & (1 << (inodeIndex % 8)))) {
                found = true;
                break;
            }
        }

        if (!found) continue;

        bitmap[inodeIndex / 8] |= (1 << (inodeIndex % 8));

        for (uint64_t i = 0; i < sectorsNeeded; i++) {
            uint64_t lba = inodeBitmapBlock * (1024ull << superblock->BlockSize) / sectorSize + i;
            if (!pdev->WriteSector(lba, (void*)((uint8_t*)bufVirt + i * sectorSize))) {
                _ds->Print("Inode bitmap write failed");
                return 0;
            }
        }

        if (GroupDescs[group]->LowFreeInodes > 0) {
            GroupDescs[group]->LowFreeInodes--;
        } else if (GroupDescs[group]->HighFreeInodes > 0) {
            GroupDescs[group]->HighFreeInodes--;
            GroupDescs[group]->LowFreeInodes = 0xFFFF;
        }

        superblock->UnallocInodes--;

        return group * superblock->InodesPerBlockGroup + inodeIndex + 1;
    }

    return 0;
}

/*
 * To Allocate a Block we must first find a free Block.
 * To do this, we can iterate through all the blocks
 * and check if it has free blocks.
 * If it does, then we can mark it as used and use it.
*/
uint32_t GenericEXT4Device::AllocateBlock() {
    uint64_t totalBlocks = ((uint64_t)superblock->HighBlocks << 32) | superblock->TotalBlocks;
    for (uint32_t i = 0; i < ceil(totalBlocks, superblock->BlocksPerBlockGroup); i++) {
        uint32_t freeBlocks = ((uint32_t)GroupDescs[i]->HighUnallocBlocks << 16) | GroupDescs[i]->LowUnallocBlocks;
        if (freeBlocks == 0) {
            continue;
        }

        uint64_t blockBitmapBlock = ((uint64_t)GroupDescs[i]->HighAddrBlockBitmap << 32) | GroupDescs[i]->LowAddrBlockBitmap;
        uint64_t blockBitmapSize = superblock->BlocksPerBlockGroup / 8;
        uint64_t sectorSize = pdev->SectorSize();
        uint64_t sectorsNeeded = ceil(blockBitmapSize, sectorSize);
        uint64_t blockSize = 1024ull << superblock->BlockSize;
        uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();
        uint64_t blockBitmapLBA = blockBitmapBlock * sectorsPerBlock;

        void* bufPhys = _ds->RequestPage();
        uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;

        _ds->MapMemory((void*)bufVirt, bufPhys, false);
        memset((void*)bufVirt, 0, 4096);

        for (uint64_t x = 0; x < sectorsNeeded; x++) {
            if (!pdev->ReadSector(blockBitmapLBA + x, (void*)((uint8_t*)bufPhys + x * sectorSize))) {
                _ds->Println("Block bitmap read failed");
                return 0;
            }
        }

        uint8_t* bitmap = (uint8_t*)bufVirt;
        uint32_t blockIndex = 0;

        uint32_t firstDataBlock = (superblock->BlockSize == 0) ? 1 : 0;
        for (int x = firstDataBlock; x < superblock->BlocksPerBlockGroup; x++) {
            blockIndex = x;
            if (!(bitmap[x / 8] & (1 << (x % 8)))) {
                break;
            }
        }

        if (blockIndex == superblock->BlocksPerBlockGroup) {
            continue;
        }

        /*
         * Mark it as used
        */
        bitmap[blockIndex / 8] |= (1 << (blockIndex % 8));

        for (uint64_t x = 0; x < sectorsNeeded; x++) {
            if (!pdev->WriteSector(blockBitmapLBA + x, (void*)((uint8_t*)bufPhys + x * sectorSize))) {
                _ds->Println("Block bitmap write failed");
                return 0;
            }
        }

        if (GroupDescs[i]->LowUnallocBlocks > 0) {
            GroupDescs[i]->LowUnallocBlocks--;
        } else if (GroupDescs[i]->HighUnallocBlocks > 0) {
            GroupDescs[i]->HighUnallocBlocks--;
            GroupDescs[i]->LowUnallocBlocks = 0xFFFF;
        }
        superblock->UnallocBlocks--;

        return i * superblock->BlocksPerBlockGroup + blockIndex;
    }

    _ds->Print("Failed to Allocate Block!");
    return 0;
}