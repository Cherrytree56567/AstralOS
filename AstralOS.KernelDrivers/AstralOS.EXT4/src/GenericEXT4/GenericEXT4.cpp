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

static bool isPower(uint32_t n, uint32_t base) {
    if (n < 1) return false;
    while (n % base == 0)
        n /= base;
    return n == 1;
}

int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* p1 = (const unsigned char*)a;
    const unsigned char* p2 = (const unsigned char*)b;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i])
            return (int)p1[i] - (int)p2[i];
    }

    return 0;
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
        if (superblock->RequiredFeatures & RequiredFeatures::BITS64) {
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

    crc32c_init_sw();

    if (superblock->FSstate & Errors) {
        switch(superblock->ErrorPolicy) {
            case Ignore:
                _ds->Println("Warning: filesystem errors detected, continuing");
                break;
            case ReadOnly:
                _ds->Println("Filesystem errors detected, mounting read-only");
                readOnly = true;
                break;
            case KernelPanic:
                _ds->Println("Filesystem errors detected, PANIC!");
                while (1);
                break;
            default:
                _ds->Println("Unknown error policy, mounting read-only");
                readOnly = true;
        }
    }

    FsNode* node = (FsNode*)_ds->malloc(sizeof(FsNode));
    if (!node) return nullptr;
    memset(node, 0, sizeof(FsNode));

    uint64_t totalBlocks = ((uint64_t)superblock->HighBlocks << 32) | superblock->TotalBlocks;

    uint32_t BlockGroupCount = (totalBlocks + superblock->BlocksPerBlockGroup - 1) / superblock->BlocksPerBlockGroup;

    GroupDescs = (BlockGroupDescriptor**)_ds->malloc(BlockGroupCount * sizeof(BlockGroupDescriptor*));
    if (!GroupDescs) return nullptr;
    memset(GroupDescs, 0, BlockGroupCount * sizeof(BlockGroupDescriptor*));
    _ds->Print("s");

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

    switch (root_inode.mode & 0xF000) {
        case 0x4000:
            node->type = FsNodeType::Directory;
            break;
        case 0x8000:
            node->type = FsNodeType::File;
            break;
        case 0xA000:
            node->type = FsNodeType::Symlink;
            break;
        default:
            node->type = FsNodeType::Unknown;
            break;
    }

    node->name = _ds->strdup("/");
    node->size = (uint64_t)root_inode.HighSize << 32 | root_inode.LowSize;
    node->blocks = root_inode.LowBlocks;

    node->mode = root_inode.mode;
    node->uid = root_inode.UserID;
    node->gid = root_inode.GroupID;

    node->atime = root_inode.LastAccess;
    node->mtime = root_inode.LastModification;
    node->ctime = root_inode.FileCreation;

    if (superblock->MountOptions32 & MountOptions::ReadOnly) {
        readOnly = true;
    }

    if ((superblock->FSstate & Errors) || (superblock->MountOptions32 & MountOptions::ReadOnly)) {
        readOnly = true;
    }

    pdev->SetMount(EXT4MountID);
    pdev->SetMountNode(node);

    //superblock->FSstate &= ~Clean;
    superblock->FSstate |= Clean; // Force Clean for now
    superblock->VolumeMountLastCheck++;
    
    if (superblock->VolumeMountLastCheck < superblock->MountsAllowedBeforeCheck) {

    } else {
        _ds->Println("Warning: Filesystem requires consistency check (fsck)!");
    }
    UpdateSuperblock();

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

    superblock->FSstate |= Clean;
    UpdateSuperblock();

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

    if (dir_inode.Flags & Extents) {
        _ds->Println("Directory Inode List Dir uses File Extents : Looong Message");
        ExtentHeader* extHdr = (ExtentHeader*)dir_inode.DBP;
        for (uint32_t i = 0; i < extHdr->entries; i++) {
            Extent* ext = (Extent*)((uint8_t*)dir_inode.DBP + sizeof(ExtentHeader) + i * sizeof(Extent));
            uint64_t startBlock = ((uint64_t)ext->startHigh << 32) | ext->startLow;
            uint64_t len = ext->len;

            for (uint64_t b = 0; b < len; b++) {
                void* bbuf = _ds->RequestPage();
                uint64_t bbufPhys = (uint64_t)bbuf;
                uint64_t bbufVirt = bbufPhys + 0xFFFFFFFF00000000;

                _ds->MapMemory((void*)bbufVirt, (void*)bbufPhys, false);
                memset((void*)bbufVirt, 0, 4096);
                uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

                for (uint64_t s = 0; s < sectorsPerBlock; s++) {
                    uint64_t lba = (startBlock + b) * sectorsPerBlock + s;
                    if (!pdev->ReadSector(lba, (void*)((uint64_t)bbufPhys + s * pdev->SectorSize()))) {
                        _ds->Println("Failed to read sector");
                        return nullptr;
                    }
                }
                DirectoryEntry* entry = (DirectoryEntry*)bbufVirt;
                uint64_t offset = 0;
                while (offset < blockSize) {
                    if (entry->Inode == 0) break;
                    char nameBuf[256] = {0};

                    if (entry->NameLen == 0 || entry->NameLen >= sizeof(nameBuf)) break;

                    memcpy(nameBuf, entry->Name, entry->NameLen);
                    nameBuf[entry->NameLen] = '\0';
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
                        child->size = childInode->LowSize | ((uint64_t)childInode->HighSize << 32);
                        child->blocks = childInode->LowBlocks;

                        child->mode = childInode->mode;
                        child->uid  = childInode->UserID;
                        child->gid  = childInode->GroupID;

                        child->atime = childInode->LastAccess;
                        child->mtime = childInode->LastModification;
                        child->ctime = childInode->FileCreation;
                    }
                    nodes[count++] = child;
                    offset += entry->TotalSize;
                    entry = (DirectoryEntry*)((uint8_t*)entry + entry->TotalSize);
                }
            }
        }
    } else {
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
                if (entry->TotalSize < sizeof(DirectoryEntry)) break;
                if (entry->Type != 0x2 && entry->Type != 0x1) break;
                if (entry->Inode == 0) {
                    offset += entry->TotalSize;
                    continue;
                }

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
                    child->size = childInode->LowSize | ((uint64_t)childInode->HighSize << 32);
                    child->blocks = childInode->LowBlocks;

                    child->mode = childInode->mode;
                    child->uid  = childInode->UserID;
                    child->gid  = childInode->GroupID;

                    child->atime = childInode->LastAccess;
                    child->mtime = childInode->LastModification;
                    child->ctime = childInode->FileCreation;
                }

                nodes[count++] = child;

                offset += entry->TotalSize;
            }
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
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsInBlock = blockSize / pdev->SectorSize();

    uint32_t InodeNum = AllocateInode(parent);
    _ds->Print("New Inode: ");
    _ds->Println(to_hstridng(InodeNum));

    uint32_t newBlock = AllocateBlock(parent);
    _ds->Print("New Block: ");
    _ds->Println(to_hstridng(newBlock));
    
    uint32_t inodeIndex = InodeNum - 1;
    uint32_t inodesPerGroup = superblock->InodesPerBlockGroup;
    uint32_t indexInGroup = inodeIndex % inodesPerGroup;
    uint64_t inodeSize = superblock->InodeSize;
    uint64_t byteOffset = indexInGroup * inodeSize;
    uint64_t offsetInBlock = byteOffset % blockSize;

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    Inode* newInode = (Inode*)bufVirt;
    newInode->mode |= 0x4000;
    newInode->mode |= 0755;

    newInode->UserID = 0;
    newInode->GroupID = 0;

    uint32_t now = 0x694792D3; // TODO: CurrentTime();
    newInode->LastAccess = now;
    newInode->LastModification = now;
    newInode->LastChange = now;
    newInode->FileCreation = now;
    newInode->Deletion = 0;

    newInode->HardLinksCount = 2;
    newInode->LowBlocks = blockSize / 512;
    newInode->Flags = 0;
    newInode->OSVal1 = 0;
    for (int i = 1; i < 15; i++) {
        newInode->DBP[i] = 0;
    }

    OSVal2 osv;
    osv.HighBlocks = 0;
    osv.HighACL = 0;
    osv.EasterEgg = 0xA574A105; // AstralOS
    osv.HighUserID = 0;
    osv.HighGroupID = 0;
    
    /*
     * TODO: Add Inode Checksum
    */
    osv.LowChecksum = 0;
    newInode->ChecksumHigh = 0;

    newInode->osval2 = osv;

    newInode->Generation = 0;
    newInode->LowACL = 0;
    newInode->FragmentBlock = 0;
    newInode->HighSize = 0;
    newInode->HighInodeSize = 0;
    newInode->HighLastChange = 0;
    newInode->HighLastModification = 0;
    newInode->HighLastAccess = 0;
    newInode->HighVersion = 0;
    newInode->ProjectID = 0;
    newInode->HighFileCreation = 0;

    newInode->LowSize = blockSize;
    WriteInode(InodeNum, newInode);

    /*
     * Our Ext4 FS can use a feature called Extents,
     * where a dir is basically a file with its contents
     * being a list of subdirs and files.
    */
    if (superblock->RequiredFeatures & RequiredFeatures::FileExtent) {
        _ds->Println("FS Requires Extents");

        ExtentHeader* extHdr = (ExtentHeader*)newInode->DBP;
        extHdr->magic = 0xF30A;
        extHdr->entries = 1;
        extHdr->max = (15 * 4 - sizeof(ExtentHeader)) / sizeof(Extent);
        extHdr->depth = 0;
        extHdr->generation = newInode->Generation;

        Extent* ext = (Extent*)(newInode->DBP + sizeof(ExtentHeader));
        ext->block = 0;
        ext->len = 1;
        ext->startLow = newBlock & 0xFFFFFFFF;
        ext->startHigh = (newBlock >> 32) & 0xFFFF;
    } else {
        _ds->Println("FS Does Not Support Extents");
        while(1);
        newInode->DBP[0] = newBlock;
    }

    DirectoryEntry* dot = (DirectoryEntry*)bufVirt;
    dot->Inode = InodeNum;
    dot->Name[0] = '.';
    dot->NameLen = 1;
    dot->Type = DETDirectory;
    dot->TotalSize = 12;
    
    DirectoryEntry* dotdot = (DirectoryEntry*)(bufVirt + dot->TotalSize);
    dotdot->Inode = parent->nodeId;
    dotdot->Name[0] = '.';
    dotdot->Name[1] = '.';
    dotdot->NameLen = 2;
    dotdot->Type = DETDirectory;
    dotdot->TotalSize = blockSize - dot->TotalSize;

    uint64_t blockLBA = newBlock * (blockSize / pdev->SectorSize());
    for (uint64_t i = 0; i < sectorsInBlock; i++) {
        if (!pdev->WriteSector(blockLBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
            _ds->Println("Failed to write directory block");
            return nullptr;
        }
    }

    _ds->Println(to_hstridng(InodeNum));
    _ds->Println("FsN");

    FsNode* fsN = (FsNode*)_ds->malloc(sizeof(FsNode));
    _ds->Println("FsN");
    memset(fsN, 0, sizeof(FsNode));
    fsN->type = FsNodeType::Directory;
    fsN->name = _ds->strdup(name);
    fsN->nodeId = InodeNum;

    fsN->size = ((uint64_t)newInode->HighSize << 32) | newInode->LowSize;
    fsN->blocks = newInode->LowBlocks;

    fsN->mode = newInode->mode;
    fsN->uid = newInode->UserID;
    fsN->gid = newInode->GroupID;

    fsN->atime = newInode->LastAccess;
    fsN->mtime = newInode->LastModification;
    fsN->ctime = newInode->FileCreation;

    Inode* parentInode = ReadInode(parent->nodeId);
    if (!parentInode) {
        _ds->Println("Failed to read parent inode");
        return nullptr;
    }

    parentInode->HardLinksCount++;
    parentInode->LastModification = now;
    parentInode->LastAccess = now;

    if ((parentInode->Flags & Extents) != 0) {
        _ds->Println("Parent Inode Uses Extents");
        ExtentHeader* extHdr = (ExtentHeader*)parentInode->DBP;
        if (extHdr->magic != 0xF30A) { 
            _ds->Println("Parent Inode has Bad Extent Header");
            return nullptr;
        }

        Extent* LastExtent = (Extent*)((uint8_t*)parentInode->DBP + sizeof(ExtentHeader) + (extHdr->entries - 1) * sizeof(Extent));
        uint32_t logicalFirst = LastExtent->block;
        uint32_t logicalLen = LastExtent->len;
        uint32_t logicalLast = logicalFirst + logicalLen - 1;

        uint64_t extentStart = ((uint64_t)LastExtent->startHigh << 32) | (uint64_t)LastExtent->startLow;
        uint64_t physicalLast = extentStart + (logicalLast - logicalFirst);

        for (uint64_t i = 0; i < sectorsInBlock; i++) {
            if (!pdev->ReadSector(physicalLast * sectorsInBlock + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                _ds->Println("Failed to read sector");
            }
        }

        bool Inserted = false;

        /*
         * I could not figure out this part so 
         * this is written with GPT
        */
        uint16_t newTotalSize = (uint16_t)((8 + strlen(name) + 3) & ~3);
        DirectoryEntry* entry = (DirectoryEntry*)bufVirt;
        while ((uint8_t*)entry < (uint8_t*)bufVirt + blockSize) {
            if (entry->TotalSize == 0) break;

            uint16_t min_len = (uint16_t)((8 + entry->NameLen + 3) & ~3);
            uint16_t slack = entry->TotalSize - min_len;

            if (slack >= newTotalSize) {
                uint16_t original_total = entry->TotalSize;
                entry->TotalSize = min_len;

                DirectoryEntry* newEntry = (DirectoryEntry*)((uint8_t*)entry + min_len);
                newEntry->Inode = InodeNum;
                newEntry->NameLen = strlen(name);
                newEntry->Type = DETDirectory;
                memcpy(newEntry->Name, name, newEntry->NameLen);
                
                newEntry->TotalSize = original_total - min_len;

                Inserted = true;
                break;
            }

            entry = (DirectoryEntry*)((uint8_t*)entry + entry->TotalSize);
        }

        if (!Inserted) {
            uint32_t newParentBlock = AllocateBlock(parent);
            _ds->Print("Allocated New Parent Block: ");
            _ds->Println(to_hstridng(newParentBlock));

            if (extHdr->entries >= extHdr->max) {
                _ds->Println("No room for new extent in inode (need tree promotion) - aborting");
                return nullptr;
            }

            Extent* newExt = (Extent*)((uint8_t*)LastExtent + sizeof(Extent));
            newExt->block = LastExtent->block + LastExtent->len;
            newExt->len = 1;
            newExt->startLow = newParentBlock & 0xFFFFFFFF;
            newExt->startHigh = (newParentBlock >> 32) & 0xFFFF;

            extHdr->entries++;

            parentInode->LowBlocks += sectorsInBlock;
            parentInode->LowSize += blockSize;

            memset((void*)bufVirt, 0, blockSize);

            DirectoryEntry* newEntry = (DirectoryEntry*)bufVirt;
            newEntry->Inode = InodeNum;
            newEntry->NameLen = strlen(name);
            newEntry->Type = DETDirectory;
            memcpy(newEntry->Name, name, newEntry->NameLen);
            newEntry->TotalSize = ((8 + newEntry->NameLen) + 3) & ~3u;

            for (uint64_t i = 0; i < sectorsInBlock; i++) {
                if (!pdev->WriteSector(newParentBlock * sectorsInBlock + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
                    _ds->Println("Failed to write sector");
                }
            }

            WriteInode(parent->nodeId, parentInode);
            return fsN;
        }

        for (uint64_t i = 0; i < sectorsInBlock; i++) {
            if (!pdev->WriteSector(physicalLast * sectorsInBlock + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                _ds->Println("Failed to write sector");
            }
        }
    }

    WriteInode(parent->nodeId, parentInode);

    return fsN;
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

    uint64_t descSize = superblock->GroupDescriptorBytes ? superblock->GroupDescriptorBytes : 64;
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

uint8_t* GenericEXT4Device::ReadBitmapInode(BlockGroupDescriptor* GroupDesc) {
    _ds->Println("Reading Inode Bitmap");
    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->HighAddrInodeBitmap << 32) | GroupDesc->LowAddrInodeBitmap;
    uint64_t inodeBitmapLBA = inodeBitmapBlock * sectorsPerBlock;

    /*
     * Now we can read our Inode Bitmap
    */
    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(inodeBitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to read inode bitmap");
            return 0;
        }
    }

    uint8_t* bitmap = (uint8_t*)bufVirt;
    return bitmap;
}

void GenericEXT4Device::WriteBitmapInode(BlockGroupDescriptor* GroupDesc, uint8_t* bitmap) {
    _ds->Println("Writing Inode Bitmap");
    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->HighAddrInodeBitmap << 32) | GroupDesc->LowAddrInodeBitmap;
    uint64_t inodeBitmapLBA = inodeBitmapBlock * sectorsPerBlock;

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(inodeBitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to read inode bitmap");
            return;
        }
    }
    
    memcpy((void*)bufVirt, bitmap, blockSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->WriteSector(inodeBitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to write inode bitmap");
            return;
        }
    }

    _ds->UnMapMemory((void*)bufVirt);
    _ds->FreePage(bufPhys);
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
uint32_t GenericEXT4Device::AllocateInode(FsNode* parent) {
    /*
     * First we need to calculate the Parent's Block Group and Index
     * so that we can get our Group Descriptor.
     * 
     * Then we can get our FlexSize and FirstFlex bc we are using Flex
     * Block Groups
    */
    uint32_t ParentBlockGroup = (parent->nodeId - 1) / superblock->InodesPerBlockGroup;

    uint32_t FlexSize = 1u << superblock->GroupsPerFlex;
    uint32_t FirstFlex = ParentBlockGroup - (ParentBlockGroup % FlexSize);

    BlockGroupDescriptor* GroupDesc = GroupDescs[FirstFlex];

    /*
     * Then we can allocate a page to use for the
     * rest of our func.
     * 
     * To re-use it, we can just memset it again.
    */
    uint8_t* bitmap = ReadBitmapInode(GroupDesc);

    uint32_t FlexInodeOff = (ParentBlockGroup - FirstFlex) * superblock->InodesPerBlockGroup;
    uint32_t First = (FirstFlex == 0) ? (superblock->FirstInode - 1) : 0;
    uint32_t newInode = 0xFFFFFFFF;

    /*
     * Now we can scan for a Free Inode
     * using our Inode Bitmap.
    */
    for (uint32_t i = FlexInodeOff; i < FlexInodeOff + superblock->InodesPerBlockGroup; i++) {
        if (i < First) continue;

        uint32_t byte = i >> 3;
        uint8_t bit = i & 7;

        if (!(bitmap[byte] & (1 << bit))) {
            bitmap[byte] |= (1 << bit);
            newInode = i;
            break;
        }
    }

    if (newInode == 0xFFFFFFFF) {
        _ds->Println("No free inode");
        return 0;
    }

    WriteBitmapInode(GroupDesc, bitmap);
    
    superblock->UnallocInodes--;
    UpdateSuperblock();
    GroupDesc->LowUnallocInodes--;
    UpdateGroupDesc(FirstFlex, GroupDesc);

    uint32_t globalInode = (ParentBlockGroup * superblock->InodesPerBlockGroup) + newInode + 1;
    return globalInode;
}

bool GenericEXT4Device::HasSuperblockBKP(uint32_t group) {
    if (group == 0 || group == 1) return true;
    if (!(superblock->ReqFeaturesRO & SparseSuperBlocks)) return true;

    if (isPower(group, 3)) return true;
    if (isPower(group, 5)) return true;
    if (isPower(group, 7)) return true;

    return false;
}

uint8_t* GenericEXT4Device::ReadBitmapBlock(BlockGroupDescriptor* GroupDesc) {
    _ds->Println("Reading Block Bitmap");
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t BitmapBlock = ((uint64_t)GroupDesc->HighAddrBlockBitmap << 32) | GroupDesc->LowAddrBlockBitmap;
    uint64_t BitmapLBA = BitmapBlock * sectorsPerBlock;

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->HighAddrInodeBitmap << 32) | GroupDesc->LowAddrInodeBitmap;

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(BitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to read block bitmap");
            return nullptr;
        }
    }

    uint8_t* Bitmap = (uint8_t*)bufVirt;

    return Bitmap;
}

void GenericEXT4Device::WriteBitmapBlock(BlockGroupDescriptor* GroupDesc, uint8_t* bitmap) {
    _ds->Println("Writing Block Bitmap");
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    uint64_t BitmapBlock = ((uint64_t)GroupDesc->HighAddrBlockBitmap << 32) | GroupDesc->LowAddrBlockBitmap;
    uint64_t BitmapLBA = BitmapBlock * sectorsPerBlock;

    uint64_t inodeBitmapBlock = ((uint64_t)GroupDesc->HighAddrInodeBitmap << 32) | GroupDesc->LowAddrInodeBitmap;

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(BitmapLBA + i, (uint8_t*)bufVirt + i * pdev->SectorSize())) {
            _ds->Println("Failed to read block bitmap");
            return;
        }
    }

    memcpy((void*)bufVirt, bitmap, blockSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->WriteSector(BitmapLBA + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
            _ds->Println("Failed to write block bitmap");
            return;
        }
    }

    _ds->UnMapMemory((void*)bufVirt);
    _ds->FreePage(bufPhys);
}

/*
 * To Allocate a Block we must first find a free Block.
 * To do this, we can iterate through all the blocks
 * and check if it has free blocks.
 * If it does, then we can mark it as used and use it.
*/
uint32_t GenericEXT4Device::AllocateBlock(FsNode* parent) {
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    /*
     * First we must find the Parent's Block Group
    */
    uint32_t parentBlockGroup = (parent->nodeId - 1) / superblock->InodesPerBlockGroup;

    if (parentBlockGroup < superblock->FirstDataBlock) {
        parentBlockGroup = superblock->FirstDataBlock;
    }

    BlockGroupDescriptor* GroupDesc = GroupDescs[parentBlockGroup];
    uint64_t inodeTableStart = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | GroupDesc->LowAddrInodeTable;
    uint64_t inodeTableBlocks = (superblock->InodesPerBlockGroup * superblock->InodeSize + blockSize - 1) / blockSize;

    uint32_t firstUsableBlock = inodeTableStart + inodeTableBlocks;

    /*
     * Get some Flex Stuff
    */
    uint32_t flexSize = 1u << superblock->GroupsPerFlex;
    uint32_t firstFlexGroup = parentBlockGroup - (parentBlockGroup % flexSize);
    uint32_t endFlexGroup = firstFlexGroup + flexSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint32_t BlockGroup = firstFlexGroup; BlockGroup < endFlexGroup; BlockGroup++) {
        BlockGroupDescriptor* GroupDesc = GroupDescs[BlockGroup];

        uint32_t freeBlocks = ((uint32_t)GroupDesc->HighUnallocBlocks << 16) | GroupDesc->LowUnallocBlocks;

        uint64_t inodeTableStart = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | GroupDesc->LowAddrInodeTable;
        uint64_t inodeTableBlocks = (superblock->InodesPerBlockGroup * superblock->InodeSize + blockSize - 1) / blockSize;

        if (freeBlocks == 0) continue;

        /*
         * Let's read our Block Bitmap
        */
        uint8_t* Bitmap = ReadBitmapBlock(GroupDesc);

        uint32_t BlocksPerGroup = superblock->BlocksPerBlockGroup;

        uint32_t maxBits = blockSize * 8;
        uint32_t limit = (BlocksPerGroup < maxBits) ? BlocksPerGroup : maxBits;

        for (uint32_t i = 0; i < limit; i++) {
            uint64_t GlobalBlock = (uint64_t)BlockGroup * BlocksPerGroup + i;
            uint64_t GroupStart = (uint64_t)BlockGroup * BlocksPerGroup;

            if (GlobalBlock == 0) continue;
            if (GlobalBlock < firstUsableBlock) continue;

            /*
             * Now we can check if it is in use
            */
            uint32_t byte_index = i / 8;
            uint8_t bit_index = i % 8;

            if (Bitmap[byte_index] & (1 << bit_index)) {
                continue;
            }

            Bitmap[byte_index] |= (1 << bit_index);

            WriteBitmapBlock(GroupDesc, Bitmap);

            superblock->UnallocBlocks--;

            uint32_t count = (GroupDesc->HighUnallocBlocks << 16) | GroupDesc->LowUnallocBlocks;
            count--;
            GroupDesc->LowUnallocBlocks = count & 0xFFFF;
            GroupDesc->HighUnallocBlocks = count >> 16;

            UpdateSuperblock();
            UpdateGroupDesc(BlockGroup, GroupDesc);

            return GlobalBlock;
        }
    }
    return 0;
}

void GenericEXT4Device::UpdateBlockBitmapChksum(uint32_t group, BlockGroupDescriptor* GroupDesc) {
    if (superblock->MetaCheckAlgo == 1) {
        uint64_t blockSize = 1024ull << superblock->BlockSize;

        uint8_t* bitmap = ReadBitmapBlock(GroupDesc);

        uint32_t crc = crc32c_sw(superblock->CheckUUID, bitmap, blockSize);

        GroupDesc->LowChkBlockBitmap = crc & 0xFFFF;
        if (superblock->RequiredFeatures & BITS64) {
            _ds->Println("64 Bit Bitmap Checksum");
            GroupDesc->HighChkBlockBitmap = (crc >> 16) & 0xFFFF;
        }
    }
}

void GenericEXT4Device::UpdateInodeBitmapChksum(uint32_t group, BlockGroupDescriptor* GroupDesc) {
    if (superblock->MetaCheckAlgo == 1) {
        uint64_t blockSize = 1024ull << superblock->BlockSize;

        uint8_t* bitmap = ReadBitmapInode(GroupDesc);

        uint32_t crc = crc32c_sw(superblock->CheckUUID, bitmap, blockSize);

        GroupDesc->LowChkInodeBitmap = crc & 0xFFFF;
        if (superblock->RequiredFeatures & BITS64) {
            _ds->Println("64 Bit Bitmap Checksum");
            GroupDesc->HighChkInodeBitmap = (crc >> 16) & 0xFFFF;
        }
    }
}

void GenericEXT4Device::UpdateGroupDesc(uint32_t group, BlockGroupDescriptor* GroupDesc) {
    _ds->Print("Group: ");
    _ds->Println(to_hstridng(group));
    if (superblock->MetaCheckAlgo == 1) {
        GroupDesc->LowChkInodeBitmap = 0;
        GroupDesc->HighChkInodeBitmap = 0;
        GroupDesc->LowChkBlockBitmap = 0;
        GroupDesc->HighChkBlockBitmap = 0;

        UpdateBlockBitmapChksum(group, GroupDesc);
        UpdateInodeBitmapChksum(group, GroupDesc);
        
        GroupDesc->Checksum = 0;

        uint32_t crc = crc32c_sw(superblock->CheckUUID, &group, sizeof(group));
        crc = crc32c_sw(crc, GroupDesc, superblock->GroupDescriptorBytes);

        GroupDesc->Checksum = crc & 0xFFFF;
    }
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / sectorSize;

    uint64_t firstGDTBlock = (blockSize == 1024) ? 2 : 1;
    uint64_t firstGDTLBA = firstGDTBlock * sectorsPerBlock;

    uint64_t descSize = superblock->GroupDescriptorBytes ? superblock->GroupDescriptorBytes : 64;

    uint64_t descByteOffset = (uint64_t)group * descSize;
    uint64_t descLBA = firstGDTLBA + (descByteOffset / sectorSize);
    uint64_t descOff = descByteOffset % sectorSize;

    uint64_t bytes = descOff + descSize;
    uint64_t sectors = (bytes + sectorSize - 1) / sectorSize;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectors; i++) {
        if (!pdev->ReadSector(descLBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("GDT read failed");
            return;
        }
    }

    memcpy((uint8_t*)bufVirt + descOff, GroupDesc, descSize);

    for (uint64_t i = 0; i < sectors; i++) {
        if (!pdev->WriteSector(descLBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("GDT write failed");
            return;
        }
    }
}

void GenericEXT4Device::UpdateSuperblock() {
    _ds->Println("Updating Superblock on Disk");
    if (superblock->MetaCheckAlgo == 1) {
        superblock->CheckUUID = crc32c_sw(~0, superblock->FilesystemUUID, sizeof(superblock->FilesystemUUID));
        superblock->Checksum = 0;
        superblock->Checksum = crc32c_sw(superblock->CheckUUID, superblock, offsetof(EXT4_Superblock, Checksum));
        _ds->Print("Updated Superblock Checksum: ");
        _ds->Println(to_hstridng(superblock->Checksum));
    }
    uint32_t sectorSize = pdev->SectorSize();

    uint64_t superblockSize = 1024;
    uint64_t SuperblockOffset = 1024;
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    _ds->Print("Block Size: ");
    _ds->Println(to_hstridng(blockSize));

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

        memcpy((uint8_t*)bufVirt + Offset, superblock, superblockSize);

        for (uint64_t i = 0; i < sectorsNeeded; i++) {
            if (!pdev->WriteSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
                _ds->Println("Failed to write superblock backup");
                return;
            }
        }
    };

    writeSuperblock(SuperblockOffset / blockSize);

    if (superblock->ReqFeaturesRO & SparseSuperBlocks != 0) {
        uint32_t totalBlockGroups = (superblock->TotalBlocks + superblock->BlocksPerBlockGroup - 1) / superblock->BlocksPerBlockGroup;
        for (uint32_t bg = 0; bg < totalBlockGroups; bg++) {
            if (bg == 0 || bg == 1 || bg % 3 == 0 || bg % 5 == 0 || bg % 7 == 0) {
                uint64_t backupBlock = bg * superblock->BlocksPerBlockGroup + superblock->FirstDataBlock;
                if (backupBlock != SuperblockOffset / blockSize) {
                    writeSuperblock(backupBlock);
                }
            }
        }
    } else {
        _ds->Println("Filesystem does not use sparse superblocks, no backups written");
    }
}

/*
 * Before, the CreateDir Func was too cluttered,
 * so now, Im making it less cluttered.
*/
void GenericEXT4Device::WriteInode(uint32_t inodeNum, Inode* ind) {
    uint64_t blockSize = 1024ull << superblock->BlockSize;
    _ds->Print(to_hstridng(inodeNum));
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / sectorSize;

    uint32_t inodeIndex = inodeNum - 1;
    uint32_t inodesPerGroup = superblock->InodesPerBlockGroup;

    uint32_t group = inodeIndex / inodesPerGroup;
    uint32_t indexInGroup = inodeIndex % inodesPerGroup;
    uint64_t inodeSize = superblock->InodeSize;
    uint64_t byteOffset = indexInGroup * inodeSize;_ds->Print("group: "); _ds->Println(to_hstridng(inodesPerGroup));

    BlockGroupDescriptor* GroupDesc = GroupDescs[group];
    if (!GroupDesc) {
        _ds->Println("Group Desc Is Invalid");
        return;
    }
    uint64_t inodeTableBlock = ((uint64_t)GroupDesc->HighAddrInodeTable << 32) | (uint64_t)GroupDesc->LowAddrInodeTable;
_ds->Print("inodeTableBlock: ");
_ds->Println(to_hstridng(inodeTableBlock));

    uint64_t blockOffset = byteOffset / blockSize;
    uint64_t offsetInBlock = byteOffset % blockSize;

    uint64_t LBA = (inodeTableBlock + blockOffset) * sectorsPerBlock;

    void* bufPhys = _ds->RequestPage();
    uint64_t bufVirt = (uint64_t)bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->ReadSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("Failed to Write Inode");
            return;
        }
    }

    memset((uint8_t*)bufVirt + offsetInBlock, 0, inodeSize);
    memcpy((uint8_t*)bufVirt + offsetInBlock, ind, inodeSize);

    for (uint64_t i = 0; i < sectorsPerBlock; i++) {
        if (!pdev->WriteSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
            _ds->Println("Failed to Write Inode [Write Sector]");
            return;
        }
    }
    _ds->Println("Wrote Inode");
}

/*
 * TODO:
 *  - Add Journal Support
*/