#include "GenericEXT4.h"
#include "../global.h"
#include <new>

size_t strlen(const char *str) {
    size_t count = 0;
    while (str[count] != '\0') {
        count++;
    }
    return count;
}

char *
strcat(char *s, const char *append)
{
	char *save = s;
	for (; *s; ++s);
	while ((*s++ = *append++) != '\0');
	return(save);
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
*/
char** str_split(DriverServices* _ds, char* a_str, const char a_delim, size_t* size) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = _ds->strdup(a_str);
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
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
    *size = count;

    return result;
}

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

bool GenericEXT4::Supports(const DeviceKey& devKey) {
    if (devKey.bars[2] == 22) {
        uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
        BaseDriver* bsdrv = (BaseDriver*)dev;
        if (bsdrv->GetDriverType() == DriverType::PartitionDevice) {
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

void GenericEXT4Device::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    uint64_t dev = ((uint64_t)devKey.bars[0] << 32) | devKey.bars[1];
    BaseDriver* bsdrv = (BaseDriver*)dev;
    pdev = (PartitionDevice*)bsdrv;
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

    if (superblock->s_magic == 0xEF53) {
        uint32_t supported = static_cast<uint32_t>(
            IncompatFeatures::INCOMPAT_FILETYPE |
            IncompatFeatures::INCOMPAT_EXTENTS |
            IncompatFeatures::INCOMPAT_64BIT |
            IncompatFeatures::INCOMPAT_FLEX_BG |
            IncompatFeatures::INCOMPAT_CSUM_SEED
        );
        if ((superblock->s_feature_incompat & ~supported) != 0) {
            _ds->Print(to_hstridng(superblock->s_feature_incompat));
            _ds->Println("Unsupported Incompat Features");
        } else {
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

    if (superblock->s_state & SuperblockState::Errors) {
        switch(superblock->s_errors) {
            case ErrorPolicy::Continue:
                _ds->Println("Warning: filesystem errors detected, continuing");
                break;
            case ErrorPolicy::ReadOnly:
                _ds->Println("Filesystem errors detected, mounting read-only");
                readOnly = true;
                break;
            case ErrorPolicy::Panic:
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

    uint64_t totalBlocks = ((uint64_t)superblock->s_blocks_count_hi << 32) | superblock->s_blocks_count_lo;

    uint32_t BlockGroupCount = (totalBlocks + superblock->s_blocks_per_group - 1) / superblock->s_blocks_per_group;

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

    switch (root_inode.i_mode & 0xF000) {
        case InodeMode::S_IFDIR:
            node->type = FsNodeType::Directory;
            break;
        case InodeMode::S_IFREG:
            node->type = FsNodeType::File;
            break;
        case InodeMode::S_IFLNK:
            node->type = FsNodeType::Symlink;
            break;
        default:
            node->type = FsNodeType::Unknown;
            break;
    }

    node->name = _ds->strdup("/");
    node->size = (uint64_t)root_inode.i_size_high << 32 | root_inode.i_size_lo;
    if (superblock->s_creator_os == CreatorOSIDs::Linux) {
        node->blocks = (uint64_t)root_inode.i_osd2.linux2.l_i_blocks_high << 32 | root_inode.i_blocks_lo;
    } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
        node->blocks = (uint64_t)root_inode.i_osd2.astral2.a_i_blocks_high << 32 | root_inode.i_blocks_lo;
    } else {
        node->blocks = root_inode.i_blocks_lo;
    }

    node->mode = root_inode.i_mode;
    node->uid = root_inode.i_uid;
    node->gid = root_inode.i_gid;

    node->atime = root_inode.i_atime;
    node->mtime = root_inode.i_mtime;
    node->ctime = root_inode.i_crtime;

    uint32_t readOnly = static_cast<uint32_t>(
        ROFeatures::RO_COMPAT_EXTRA_ISIZE | 
        ROFeatures::RO_COMPAT_SPARSE_SUPER |
        ROFeatures::RO_COMPAT_DIR_NLINK | 
        ROFeatures::RO_COMPAT_HUGE_FILE | 
        ROFeatures::RO_COMPAT_LARGE_FILE |
        ROFeatures::RO_COMPAT_METADATA_CSUM
    );

    uint32_t CompatFeatures = static_cast<uint32_t>(
        CompatibleFeatures::COMPAT_HAS_JOURNAL
    );

    if ((superblock->s_state & SuperblockState::Errors) || (superblock->s_feature_ro_compat & ~readOnly)) {
        _ds->Println("ReadONLY");
        readOnly = true;
    }

    pdev->SetMount(EXT4MountID);
    pdev->SetMountNode(node);
    rootNode = *node;

    //superblock->FSstate &= ~Clean;
    superblock->s_state |= SuperblockState::Clean; // Force Clean for now
    superblock->s_mnt_count++;
    
    if (superblock->s_mnt_count > superblock->s_max_mnt_count) {
        _ds->Println("Warning: Filesystem requires consistency check (fsck)!");
    }

    if (superblock->s_feature_compat & ~CompatFeatures) {
        _ds->Println("Warning: EXT4 Optional Features not supported.");
    }

    UpdateSuperblock();

    isMounted = true;

    return node;
}

bool GenericEXT4Device::Unmount() {
    if (!isMounted) {
        return false;
    }

    superblock->s_state |= SuperblockState::Clean;
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
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsInBlock = blockSize / pdev->SectorSize();
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return nullptr;
    }
    if (node->type != FsNodeType::Directory) {
        return nullptr;
    }

    Inode dir_inode = *ReadInode(node->nodeId);

    FsNode** nodes = nullptr;
    uint64_t count = 0;
    size_t capacity = 0;

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

    if ((dir_inode.i_flags & InodeFlags::EXT4_EXTENTS_FL) != 0) {
        ExtentHeader* eh = (ExtentHeader*)dir_inode.i_block;

        uint64_t ExtsCount = 0;
        Extent** extents = GetExtents(eh, ExtsCount);

        for (int i = 0; i < ExtsCount; i++) {
            Extent* ee = extents[i];

            if (ee->ee_len == 0) continue;

            uint64_t block = ((uint64_t)ee->ee_start_hi << 32) | (uint64_t)ee->ee_start_lo;
            
            ParseDirectoryBlock(nodes, count, capacity, block);
        }
    } else {
        for (uint64_t i = 0; i < 15; i++) {
            if (dir_inode.i_block[i] == 0) continue;
            
            ParseDirectoryBlock(nodes, count, capacity, dir_inode.i_block[i]);
        }
    }

    *outCount = count;
    return nodes;
}

/*
 * To Find a File through a Dir you can just
 * use `ListDir` to get all the files in the
 * node and recursively look through subdirs
 * to find the file and return the FsNode*.
*/
FsNode* GenericEXT4Device::FindDir(FsNode* node, const char* path) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return nullptr;
    }
    size_t cont = 0;

    char** parts = str_split(_ds, _ds->strdup(path), '/', &cont);

    FsNode* nd = node;

    for (int i = 0; i < cont; i++) {
        bool found = false;
        size_t count = 0;
        FsNode** contents = ListDir(nd, &count);

        for (int x = 0; x < count; x++) {
            if (strcmp(contents[x]->name, parts[i]) == 0) {
                nd = contents[x];
            }
            found = true;
        }
        if (found != true) {
            _ds->Println("File Not Found");
            return nullptr;
        }
    }
    
    return nd;
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
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return nullptr;
    }
    if (readOnly) {
        _ds->Println("Can't Create Dir, Read Only");
        return nullptr;
    }
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsInBlock = blockSize / pdev->SectorSize();

    uint32_t InodeNum = AllocateInode(parent);
    uint32_t newBlock = AllocateBlock(parent);
    
    uint32_t inodeIndex = InodeNum - 1;
    uint32_t inodesPerGroup = superblock->s_inodes_per_group;
    uint32_t indexInGroup = inodeIndex % inodesPerGroup;
    uint64_t inodeSize = superblock->s_inode_size;
    uint64_t byteOffset = indexInGroup * inodeSize;
    uint64_t offsetInBlock = byteOffset % blockSize;

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    Inode* newInode = (Inode*)_ds->malloc(sizeof(Inode));
    newInode->i_mode |= 0x4000;
    newInode->i_mode |= 0755;

    newInode->i_uid = 0;
    newInode->i_gid = 0;

    uint32_t now = 0xFFAABBCC; // TODO: CurrentTime();
    newInode->i_atime = now;
    newInode->i_mtime = now;
    newInode->i_ctime = now;
    newInode->i_crtime = now;
    newInode->i_dtime = 0;

    newInode->i_links_count = 2;
    newInode->i_blocks_lo = blockSize / 512;
    newInode->i_flags = 0;
    newInode->i_osd1.linux1.l_i_version = 0;
    for (int i = 0; i < 15; i++) {
        newInode->i_block[i] = 0;
    }

    /*
     * TODO: Add Inode Checksum
    */
    if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
        newInode->i_osd2.astral2.a_i_blocks_high = 0;
        newInode->i_osd2.astral2.a_i_checksum_lo = 0;
        newInode->i_osd2.astral2.a_i_easteregg = 0xA574A105;
        newInode->i_osd2.astral2.a_i_uid_high = 0;
        newInode->i_osd2.astral2.a_i_gid_high = 0;
    } else if (superblock->s_creator_os == CreatorOSIDs::Linux) {
        newInode->i_osd2.linux2.l_i_blocks_high = 0;
        newInode->i_osd2.linux2.l_i_checksum_lo = 0;
        newInode->i_osd2.linux2.l_i_reserved = 0;
        newInode->i_osd2.linux2.l_i_uid_high = 0;
        newInode->i_osd2.linux2.l_i_gid_high = 0;
    } else if (superblock->s_creator_os == CreatorOSIDs::GNUHurd) {
        newInode->i_osd2.hurd2.h_i_author = 0;
        newInode->i_osd2.hurd2.h_i_gid_high = 0;
        newInode->i_osd2.hurd2.h_i_mode_high = 0;
        newInode->i_osd2.hurd2.h_i_reserved1 = 0;
        newInode->i_osd2.hurd2.h_i_uid_high = 0;
    } else if (superblock->s_creator_os == CreatorOSIDs::MASIX) {
        newInode->i_osd2.masix2.m_i_file_acl_high = 0;
        newInode->i_osd2.masix2.h_i_reserved1 = 0;
        newInode->i_osd2.masix2.m_i_reserved2[0] = 0;
        newInode->i_osd2.masix2.m_i_reserved2[1] = 0;
    }
    
    
    newInode->i_checksum_hi = 0;

    newInode->i_generation = 0;
    newInode->i_file_acl_lo = 0;
    newInode->i_obso_faddr = 0;
    newInode->i_size_high = 0;
    newInode->i_extra_isize = superblock->s_inode_size - 128;
    newInode->i_ctime_extra = 0;
    newInode->i_mtime_extra = 0;
    newInode->i_atime_extra = 0;
    newInode->i_version_hi = 0;
    newInode->i_projid = 0;
    newInode->i_crtime_extra = 0;

    newInode->i_size_lo = blockSize;

    /*
     * Our Ext4 FS can use a feature called Extents,
     * where a dir is basically a file with its contents
     * being a list of subdirs and files.
    */
    if (superblock->s_feature_incompat & IncompatFeatures::INCOMPAT_EXTENTS) {
        ExtentHeader* extHdr = (ExtentHeader*)newInode->i_block;
        _ds->Print(to_hstridng(extHdr->eh_magic));
        extHdr->eh_magic = 0xF30A;
        extHdr->eh_entries = 1;
        extHdr->eh_max = 4;
        extHdr->eh_depth = 0;
        extHdr->eh_generation = newInode->i_generation;

        Extent* ext = (Extent*)((uint8_t*)newInode->i_block + sizeof(ExtentHeader));
        ext->ee_block = 0x0;
        ext->ee_len = 1;
        ext->ee_start_lo = newBlock & 0xFFFFFFFF;
        ext->ee_start_hi = (newBlock >> 32) & 0xFFFF;

        newInode->i_flags |= InodeFlags::EXT4_EXTENTS_FL;
    } else {
        _ds->Println("FS Does Not Support Extents");
        newInode->i_block[0] = newBlock;
    }

    WriteInode(InodeNum, newInode);
/*
    
    for (uint64_t i = 0; i < sectorsInBlock; i++) {
        if (!pdev->ReadSector(blockLBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
            _ds->Println("Failed to read directory block");
            return nullptr;
        }
    }*/

    DirectoryEntry2* dot = (DirectoryEntry2*)bufVirt;
    dot->inode = InodeNum;
    dot->name[0] = '.';
    dot->name_len = 1;
    dot->file_type = DirectoryEntryType::EXT4_FT_DIR;
    int rec_len = (1 + 8 + 3);
	rec_len &= ~3;
    dot->rec_len = rec_len;
    
    DirectoryEntry2* dotdot = (DirectoryEntry2*)(bufVirt + dot->rec_len);
    dotdot->inode = parent->nodeId;
    dotdot->name[0] = '.';
    dotdot->name[1] = '.';
    dotdot->name_len = 2;
    dotdot->file_type = DirectoryEntryType::EXT4_FT_DIR;
    /*
     * From e2fsprogs
     * https://github.com/tytso/e2fsprogs/blob/master/lib/ext2fs/newdir.c#L74
    */
    if (((blockSize - dot->rec_len) > blockSize) || (blockSize > (1 << 18)) || ((blockSize - dot->rec_len) & 3)) {
		dotdot->rec_len = 0;
	}

    if ((blockSize - dot->rec_len) < 65536) {
		dotdot->rec_len = (blockSize - dot->rec_len);
	}

	if ((blockSize - dot->rec_len) == blockSize) {
		if (blockSize == 65536) {
			dotdot->rec_len = ((1<<16)-1);
        } else {
			dotdot->rec_len = 0;
        }
	} else {
		dotdot->rec_len = ((blockSize - dot->rec_len) & 65532) | (((blockSize - dot->rec_len) >> 16) & 3);
    }

    uint64_t blockLBA = newBlock * (blockSize / pdev->SectorSize());
    for (uint64_t i = 0; i < sectorsInBlock; i++) {
        if (!pdev->WriteSector(blockLBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
            _ds->Println("Failed to write directory block");
            return nullptr;
        }
    }

    FsNode* fsN = (FsNode*)_ds->malloc(sizeof(FsNode));
    memset(fsN, 0, sizeof(FsNode));
    fsN->type = FsNodeType::Directory;
    fsN->name = _ds->strdup(name);
    fsN->nodeId = InodeNum;

    fsN->size = ((uint64_t)newInode->i_size_high << 32) | newInode->i_size_lo;
    if (superblock->s_creator_os == CreatorOSIDs::Linux) {
        fsN->blocks = (uint64_t)newInode->i_osd2.linux2.l_i_blocks_high << 32 | newInode->i_blocks_lo;
    } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
        fsN->blocks = (uint64_t)newInode->i_osd2.astral2.a_i_blocks_high << 32 | newInode->i_blocks_lo;
    } else {
        fsN->blocks = newInode->i_blocks_lo;
    }

    fsN->mode = newInode->i_mode;
    fsN->uid = newInode->i_uid;
    fsN->gid = newInode->i_gid;

    fsN->atime = newInode->i_atime;
    fsN->mtime = newInode->i_mtime;
    fsN->ctime = newInode->i_crtime;

    Inode* parentInode = ReadInode(parent->nodeId);
    if (!parentInode) {
        _ds->Println("Failed to read parent inode");
        return nullptr;
    }

    parentInode->i_links_count++;
    parentInode->i_mtime = now;
    parentInode->i_atime = now;

    if ((parentInode->i_flags & InodeFlags::EXT4_EXTENTS_FL) != 0) {
        ExtentHeader* extHdr = (ExtentHeader*)parentInode->i_block;
        if (extHdr->eh_magic != 0xF30A) { 
            _ds->Println("Parent Inode has Bad Extent Header");
            return nullptr;
        }

        Extent* LastExtent = (Extent*)((uint8_t*)parentInode->i_block + sizeof(ExtentHeader) + (extHdr->eh_entries - 1) * sizeof(Extent));
        uint32_t logicalFirst = LastExtent->ee_block;
        uint32_t logicalLen = LastExtent->ee_len;
        uint32_t logicalLast = logicalFirst + logicalLen - 1;

        uint64_t extentStart = ((uint64_t)LastExtent->ee_start_hi << 32) | (uint64_t)LastExtent->ee_start_lo;
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
        DirectoryEntry2* entry = (DirectoryEntry2*)bufVirt;
        while ((uint8_t*)entry < (uint8_t*)bufVirt + blockSize) {
            if (entry->rec_len == 0) break;

            uint16_t min_len = (uint16_t)((8 + entry->name_len + 3) & ~3);
            uint16_t slack = entry->rec_len - min_len;

            if (slack >= newTotalSize) {
                uint16_t original_total = entry->rec_len;
                entry->rec_len = min_len;

                DirectoryEntry2* newEntry = (DirectoryEntry2*)((uint8_t*)entry + min_len);
                newEntry->inode = InodeNum;
                newEntry->name_len = strlen(name);
                newEntry->file_type = DirectoryEntryType::EXT4_FT_DIR;
                memcpy(newEntry->name, name, newEntry->name_len);
                
                newEntry->rec_len = original_total - min_len;

                Inserted = true;
                break;
            }

            entry = (DirectoryEntry2*)((uint8_t*)entry + entry->rec_len);
        }

        if (!Inserted) {
            uint32_t newParentBlock = AllocateBlock(parent);

            if (extHdr->eh_entries >= extHdr->eh_max) {
                _ds->Println("No room for new extent in inode (need tree promotion) - aborting");
                return nullptr;
            }

            Extent* newExt = (Extent*)((uint8_t*)LastExtent + sizeof(Extent));
            newExt->ee_block = LastExtent->ee_block + LastExtent->ee_len;
            newExt->ee_len = 1;
            newExt->ee_start_lo = newParentBlock & 0xFFFFFFFF;
            newExt->ee_start_hi = (newParentBlock >> 32) & 0xFFFF;

            extHdr->eh_entries++;

            parentInode->i_blocks_lo += sectorsInBlock;
            parentInode->i_size_lo += blockSize;

            memset((void*)bufVirt, 0, blockSize);

            DirectoryEntry2* newEntry = (DirectoryEntry2*)bufVirt;
            newEntry->inode = InodeNum;
            newEntry->name_len = strlen(name);
            newEntry->file_type = DirectoryEntryType::EXT4_FT_DIR;
            memcpy(newEntry->name, name, newEntry->name_len);
            newEntry->rec_len = ((8 + newEntry->name_len) + 3) & ~3u;

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

    uint32_t group = (parent->nodeId - 1) / superblock->s_inodes_per_group;
    BlockGroupDescriptor* GroupDesc = GroupDescs[group];

    uint32_t usedDirs = ((uint32_t)GroupDesc->bg_used_dirs_count_hi << 16) | GroupDesc->bg_used_dirs_count_lo;

    usedDirs++;

    GroupDesc->bg_used_dirs_count_lo = usedDirs & 0xFFFF;
    GroupDesc->bg_used_dirs_count_hi = usedDirs >> 16;

    UpdateGroupDesc(group, GroupDesc);
    WriteInode(parent->nodeId, parentInode);

    return fsN;
}

bool GenericEXT4Device::Remove(FsNode* node) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return false;
    }
    if (node->type == FsNodeType::Directory) {
        if (node->nodeId == 2) {
            _ds->Println("Cannot remove root directory");
            return false;
        }

        size_t count = 0;
        FsNode** contents = ListDir(node, &count);
        if (count > 2) {
            _ds->Println("Directory not empty, cannot remove");
            _ds->free(contents);
            return false;
        }
        uint64_t parent = 0;
        for (int i = 0; i < 2; i++) {
            if (strcmp(contents[i]->name, "..") == 0) {
                parent = contents[i]->nodeId;
                break;
            }
        }
        
        _ds->free(contents);

        Inode parent_inode = *ReadInode(parent);
    }
}

File* GenericEXT4Device::Open(const char* name, uint32_t flags) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return nullptr;
    }
    File* file = (File*)_ds->malloc(sizeof(File));
    if (!file) {
        _ds->Println("Failed to allocate File struct");
        return nullptr;
    }

    bool write = flags & (WR | APPEND | CREATE);

    if (flags & CREATE) {
        _ds->Println("Creating File");
        _ds->Println(to_hstridng(flags));
        file->node = (FsNode*)_ds->malloc(sizeof(FsNode));
        file->node->nodeId = 0;
        file->node->name = _ds->strdup(name);
    } else {
        FsNode* node = FindDir(&rootNode, name);
        Inode* inode = ReadInode(node->nodeId);

        if ((inode->i_flags & InodeFlags::EXT4_IMMUTABLE_FL) && write) {
            _ds->Println("Cannot open immutable file for writing");
            return nullptr;
        }

        if ((inode->i_flags & InodeFlags::EXT4_APPEND_FL) && write && !(flags & APPEND)) {
            _ds->Println("Cannot open append-only file for writing without APPEND flag");
            return nullptr;
        }

        if (node->type == FsNodeType::Directory && write) {
            _ds->Println("Cannot open directory for writing");
            return nullptr;
        }
        if ((flags & TRUNC) && node->type != FsNodeType::File) {
            _ds->Println("Cannot truncate non-file node");
            return nullptr;
        }

        _ds->Print("Opening File: ");
        _ds->Println(to_hstridng(node->nodeId));

        file->node = node;
    }

    if (write && readOnly) {
        _ds->Println("Cannot Write when Read Only Mode is enabled!");
        return nullptr;
    }

    file->flags = flags;
    file->position = 0;
    return file;
}

bool GenericEXT4Device::Close(File* file) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return false;
    }
    _ds->free(file->node);
    _ds->free(file);
    file = nullptr;
    return true;
}

/*
 * Some of it is GPT Generated
*/
int64_t GenericEXT4Device::Read(File* file, void* buffer, uint64_t size) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return -1;
    }
    if (!file) return -3;

    if (buffer == nullptr && size == 0) {
        Inode* inode = ReadInode(file->node->nodeId);
        uint64_t fileSize = inode->i_size_lo | ((uint64_t)inode->i_size_high << 32);

        if (file->position >= fileSize) return 0;

        return fileSize - file->position;
    }

    if (!(file->flags & RD) && !(file->flags & (RD | WR))) {
        _ds->Println("Read: File not opened for reading");
        return -1;
    }
    if (file->node->type == FsNodeType::Directory) {
        _ds->Println("Read: Cannot read from a directory");
        return -2;
    }
    
    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsInBlock = blockSize / pdev->SectorSize();

    void* buf = _ds->RequestPage();
    uint64_t bufPhys = (uint64_t)buf;
    uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
    _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
    memset((void*)bufVirt, 0, 4096);

    FsNode* node = file->node;

    Inode* inode = ReadInode(node->nodeId);

    uint64_t fileSize = inode->i_size_lo | ((uint64_t)inode->i_size_high << 32);
    uint64_t pos = file->position;

    if (pos >= fileSize) {
        return 0;
    }

    uint64_t toRead = size;
    if (pos + toRead > fileSize) toRead = fileSize - pos;

    uint8_t* out = (uint8_t*)buffer;
    uint64_t readTotal = 0;

    if ((inode->i_flags & InodeFlags::EXT4_EXTENTS_FL)) {
        ExtentHeader* eh = (ExtentHeader*)inode->i_block;

        uint64_t EXTsCount = 0;
        Extent** exts = GetExtents(eh, EXTsCount);

        for (int i = 0; i < EXTsCount; i++) {
            Extent* ee = exts[i];

            if (ee->ee_len == 0) continue;

            uint64_t block = ((uint64_t)ee->ee_start_hi << 32) | (uint64_t)ee->ee_start_lo;
            uint64_t fileBlock = ee->ee_block;

            for (int x = 0; x < ee->ee_len; x++) {
                memset((void*)bufVirt, 0, 4096);
                uint64_t LBA = (block + x) * sectorsInBlock;
                    
                for (uint64_t j = 0; j < sectorsInBlock; j++) {
                    if (!pdev->ReadSector(LBA + j, (void*)((uint8_t*)bufPhys + j * pdev->SectorSize()))) {
                        _ds->Println("Failed to read directory block");
                        return -28;
                    }
                }

                uint64_t off = (fileBlock + x) * blockSize;

                uint64_t remaining = fileSize - off;
                uint64_t toCopy = remaining < blockSize ? remaining : blockSize;

                memcpy(out + off, (void*)bufVirt, toCopy);

                readTotal += blockSize;
            }
        }
    } else {
        _ds->Println("File Doesnt use Extents!");
        
        uint64_t remaining = toRead;
        while (remaining > 0) {
            uint64_t blockIndex = pos / blockSize;
            uint64_t blockOffset = pos % blockSize;

            if (blockIndex >= 12) break;

            uint32_t block = inode->i_block[blockIndex];
            if (block == 0) break;

            uint64_t LBA = block * sectorsInBlock;

            for (uint64_t i = 0; i < sectorsInBlock; i++) {
                if (!pdev->ReadSector(LBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                    _ds->Println("Failed to read directory block");
                    return -28;
                }
            }

            uint64_t chunk = blockSize - blockOffset;
            if (chunk > remaining) chunk = remaining;

            memcpy(out + readTotal, (uint8_t*)bufVirt + blockOffset, chunk);

            readTotal += chunk;
            pos += chunk;
            remaining -= chunk;
        }
    }

    return readTotal;
}

int64_t GenericEXT4Device::Write(File* file, void* buffer, uint64_t size) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return -1;
    }
    if (readOnly) {
        _ds->Println("Can't Write, Read Only");
        return -1;
    }

    uint64_t blockSize = 1024ull << superblock->s_log_block_size;
    uint64_t sectorSize = pdev->SectorSize();
    uint64_t sectorsPerBlock = blockSize / pdev->SectorSize();

    if ((file->flags & (WR | APPEND)) == (WR | APPEND)) {
        Inode* inode = ReadInode(file->node->nodeId);

        void* buf = _ds->RequestPage();
        uint64_t bufPhys = (uint64_t)buf;
        uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
        _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);

        uint64_t fileSize = file->node->size;

        /*
         * We can use tailOffset to know if all the blocks
         * are full and if we need to allocate a new block
         * or not.
        */
        uint64_t tailOffset = fileSize % blockSize;

        bool cpy = false;

        if (tailOffset != 0) {
            ExtentHeader* exHdr = (ExtentHeader*)inode->i_block;
            uint64_t extsCount = 0;
            Extent** exts = GetExtents(exHdr, extsCount);

            /*
             * Get the last avaliable block and read it
            */
            uint64_t block = ((uint64_t)exts[extsCount - 1]->ee_start_hi << 32) | (uint64_t)exts[extsCount - 1]->ee_start_lo;

            uint64_t LBA = block * sectorsPerBlock;

            for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                if (!pdev->ReadSector(LBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                    _ds->Println("Failed to read file block");
                    return -28;
                }
            }

            _ds->Print("File Offset: ");
            _ds->Println(to_hstridng(tailOffset));
            _ds->Print("Block: ");
            _ds->Println(to_hstridng(block));
            _ds->Print("Extents Count: ");
            _ds->Println(to_hstridng(extsCount));

            /*
             * Calculate if we need to allocate a new block
             * or just use the current one.
            */
            uint64_t spaceLeft = blockSize - tailOffset;

            uint64_t toCopy;

            if (size <= spaceLeft) {
                toCopy = size;
            } else {
                _ds->Println("SpacLeft");
                toCopy = spaceLeft;
            }

            memcpy((uint8_t*)bufVirt + tailOffset, buffer, toCopy);

            for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                if (!pdev->WriteSector(LBA + i, (uint8_t*)bufPhys + i * sectorSize)) {
                    return -28;
                }
            }

            /*
             * Return if don't need to copy
             * data to a newly alloc'ed block.
            */
            if (size <= spaceLeft) {
                uint64_t newSize = file->node->size + size;
                inode->i_size_lo = newSize & 0xFFFFFFFF;
                inode->i_size_high = newSize >> 32;
                file->node->size = newSize;

                WriteInode(file->node->nodeId, inode);
                return size;
            } else {
                cpy = true;
            }
        }

        uint64_t remaining = size - (blockSize - tailOffset);
        uint64_t blocksNeeded = (remaining + blockSize - 1) / blockSize;

        uint64_t superblockCount = ((uint64_t)superblock->s_free_blocks_count_hi << 32) | superblock->s_free_blocks_count_lo;

        uint64_t bufOff = 0;

        if (tailOffset == 0) {
            remaining = size;
            blocksNeeded = (remaining + blockSize - 1) / blockSize;
        }

        while (true) {
            if (remaining == 0) break;
            if (blocksNeeded == 0) break;

            uint64_t countc = blocksNeeded;
            uint64_t blocks = AllocateBlocks(file->node, countc);

            uint64_t count = blocksNeeded - countc;

            _ds->Print("Count C: ");
            _ds->Println(to_hstridng(countc));
            _ds->Print("Count: ");
            _ds->Println(to_hstridng(count));
            _ds->Print("Blocks: ");
            _ds->Println(to_hstridng(blocks));
            _ds->Print("Blocks Needed: ");
            _ds->Println(to_hstridng(blocksNeeded));
            _ds->Print("Remaining: ");
            _ds->Println(to_hstridng(remaining));

            Extent ee;
            ee.ee_block = file->node->size / blockSize;
            ee.ee_len = count;
            ee.ee_start_lo = (uint32_t)(blocks & 0xFFFFFFFF);
            ee.ee_start_hi = (uint16_t)((blocks >> 32) & 0xFFFF);

            _ds->Print("Block: ");
            _ds->Println(to_hstridng(ee.ee_block));
            _ds->Print("Len: ");
            _ds->Println(to_hstridng(ee.ee_len));
            _ds->Print("Start Low: ");
            _ds->Println(to_hstridng(ee.ee_start_lo));
            _ds->Print("Start High: ");
            _ds->Println(to_hstridng(ee.ee_start_hi));

            /*
             * The Number of bytes in the file that
             * we write to the blocks
            */
            uint64_t BlockBytes = count * blockSize;
            for (uint64_t i = 0; i < count; i++) {
                memset((void*)bufVirt, 0, 4096);

                if (remaining < blockSize) {
                    memcpy((uint8_t*)bufVirt, (uint8_t*)buffer + bufOff, remaining);
                    bufOff += remaining;
                } else {
                    memcpy((uint8_t*)bufVirt, (uint8_t*)buffer + bufOff, blockSize);
                    bufOff += blockSize;
                }

                uint64_t LBA = (i + blocks) * sectorsPerBlock;

                for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                    if (!pdev->WriteSector(LBA + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                        _ds->Println("Failed to read file block");
                        return -28;
                    }
                }
            }

            if (remaining < BlockBytes) {
                remaining -= remaining;
            } else {
                remaining -= BlockBytes;
            }
            blocksNeeded -= count;

            if (!AddExtent(file->node, inode, ee)) {
                _ds->Println("Failed to Add Extent");
            }

            uint64_t blocksCount = (uint64_t)inode->i_blocks_lo;

            if (superblock->s_creator_os == CreatorOSIDs::Linux) {
                blocksCount = ((uint64_t)inode->i_osd2.linux2.l_i_blocks_high << 32) | inode->i_blocks_lo;
            } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
                blocksCount = ((uint64_t)inode->i_osd2.astral2.a_i_blocks_high << 32) | inode->i_blocks_lo;
            }

            blocksCount += count * sectorsPerBlock;

            inode->i_blocks_lo = blocksCount & 0xFFFFFFFF;

            if (superblock->s_creator_os == CreatorOSIDs::Linux) {
                inode->i_osd2.linux2.l_i_blocks_high = blocksCount >> 32;
            } else if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
                inode->i_osd2.astral2.a_i_blocks_high = blocksCount >> 32;
            }

            superblockCount -= count;
        }

        uint64_t newSize = file->node->size + size;
        inode->i_size_lo = newSize & 0xFFFFFFFF;
        inode->i_size_high = newSize >> 32;
        file->node->size = newSize;

        WriteInode(file->node->nodeId, inode);

        superblock->s_free_blocks_count_hi = superblockCount >> 32;
        superblock->s_free_blocks_count_lo = superblockCount & 0xFFFFFFFF;
        UpdateSuperblock();

        return size;
    } else if ((file->flags & CREATE)) {
        if (file->node->type == FsNodeType::File) {
            size_t size = 0;
            char** p = str_split(_ds, _ds->strdup(file->node->name), '/', &size);

            uint64_t newSize = 0;
            for (size_t i = 0; i < (size - 1); i++) {
                if ((size - 1) == 1) {
                    newSize += strlen(p[i - 1]);
                } else {
                    newSize += strlen(p[i - 1]) + 1;
                }
            }

            char* newPath = (char*)_ds->malloc(newSize);
            
            for (size_t i = 0; i < (size - 1); i++) {
                if ((size - 1) == 1) {
                    strcat(newPath, p[i]);
                } else {
                    strcat(newPath, p[i]);
                    strcat(newPath, "/");
                }
            }

            char* newFile = p[size - 1];

            _ds->Print("Create File: ");
            _ds->Println(newPath);
            _ds->Println(newFile);

            FsNode* fsN = FindDir(pdev->GetMountNode(), newPath);
            if (!fsN) {
                _ds->Println("Failed to Find Dir");
                return -1;
            }

            uint32_t InodeNum = AllocateInode(fsN);
            
            uint32_t inodeIndex = InodeNum - 1;
            uint32_t inodesPerGroup = superblock->s_inodes_per_group;
            uint32_t indexInGroup = inodeIndex % inodesPerGroup;
            uint64_t inodeSize = superblock->s_inode_size;
            uint64_t byteOffset = indexInGroup * inodeSize;
            uint64_t offsetInBlock = byteOffset % blockSize;

            void* buf = _ds->RequestPage();
            uint64_t bufPhys = (uint64_t)buf;
            uint64_t bufVirt = bufPhys + 0xFFFFFFFF00000000;
            _ds->MapMemory((void*)bufVirt, (void*)bufPhys, false);
            memset((void*)bufVirt, 0, 4096);

            Inode* newInode = (Inode*)_ds->malloc(sizeof(Inode));
            newInode->i_mode |= InodeMode::S_IFREG;
            newInode->i_mode |= 0755;

            newInode->i_uid = 0;
            newInode->i_gid = 0;

            uint32_t now = 0xFFAABBCC; // TODO: CurrentTime();
            newInode->i_atime = now;
            newInode->i_mtime = now;
            newInode->i_ctime = now;
            newInode->i_crtime = now;
            newInode->i_dtime = 0;

            newInode->i_links_count = 1;
            newInode->i_blocks_lo = 0;
            newInode->i_flags = 0;
            newInode->i_osd1.linux1.l_i_version = 0;
            for (int i = 0; i < 15; i++) {
                newInode->i_block[i] = 0;
            }

            /*
            * TODO: Add Inode Checksum
            */
            if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
                newInode->i_osd2.astral2.a_i_blocks_high = 0;
                newInode->i_osd2.astral2.a_i_checksum_lo = 0;
                newInode->i_osd2.astral2.a_i_easteregg = 0xA574A105;
                newInode->i_osd2.astral2.a_i_uid_high = 0;
                newInode->i_osd2.astral2.a_i_gid_high = 0;
            } else if (superblock->s_creator_os == CreatorOSIDs::Linux) {
                newInode->i_osd2.linux2.l_i_blocks_high = 0;
                newInode->i_osd2.linux2.l_i_checksum_lo = 0;
                newInode->i_osd2.linux2.l_i_reserved = 0;
                newInode->i_osd2.linux2.l_i_uid_high = 0;
                newInode->i_osd2.linux2.l_i_gid_high = 0;
            } else if (superblock->s_creator_os == CreatorOSIDs::GNUHurd) {
                newInode->i_osd2.hurd2.h_i_author = 0;
                newInode->i_osd2.hurd2.h_i_gid_high = 0;
                newInode->i_osd2.hurd2.h_i_mode_high = 0;
                newInode->i_osd2.hurd2.h_i_reserved1 = 0;
                newInode->i_osd2.hurd2.h_i_uid_high = 0;
            } else if (superblock->s_creator_os == CreatorOSIDs::MASIX) {
                newInode->i_osd2.masix2.m_i_file_acl_high = 0;
                newInode->i_osd2.masix2.h_i_reserved1 = 0;
                newInode->i_osd2.masix2.m_i_reserved2[0] = 0;
                newInode->i_osd2.masix2.m_i_reserved2[1] = 0;
            }
            
            newInode->i_checksum_hi = 0;

            newInode->i_generation = 0;
            newInode->i_file_acl_lo = 0;
            newInode->i_obso_faddr = 0;
            newInode->i_size_high = 0;
            newInode->i_extra_isize = superblock->s_inode_size - 128;
            newInode->i_ctime_extra = 0;
            newInode->i_mtime_extra = 0;
            newInode->i_atime_extra = 0;
            newInode->i_version_hi = 0;
            newInode->i_projid = 0;
            newInode->i_crtime_extra = 0;

            newInode->i_size_lo = 0;

            /*
            * Our Ext4 FS can use a feature called Extents,
            * where a dir is basically a file with its contents
            * being a list of subdirs and files.
            */
            if (superblock->s_feature_incompat & IncompatFeatures::INCOMPAT_EXTENTS) {
                uint32_t newBlk = AllocateBlock(fsN);
                
                ExtentHeader* extHdr = (ExtentHeader*)((uint8_t*)newInode->i_block);
                _ds->Print(to_hstridng(extHdr->eh_magic));
                extHdr->eh_magic = 0xF30A;
                extHdr->eh_entries = 0;
                extHdr->eh_max = 4;
                extHdr->eh_depth = 0;
                extHdr->eh_generation = newInode->i_generation;

                newInode->i_flags = InodeFlags::EXT4_EXTENTS_FL;
            }

            WriteInode(InodeNum, newInode);

            Inode* ParentInode = ReadInode(fsN->nodeId);

            ExtentHeader* hdr = (ExtentHeader*)((uint8_t*)ParentInode->i_block);

            if (hdr->eh_magic != 0xF30A) { 
                _ds->Println("Parent Inode has Bad Extent Header");
                return -2;
            }

            uint64_t ExtentsCount = 0;
            Extent** exts = GetExtents(hdr, ExtentsCount);

            Extent* LastExtent = exts[ExtentsCount - 1];
            uint32_t logicalFirst = LastExtent->ee_block;
            uint32_t logicalLen = LastExtent->ee_len;
            uint32_t logicalLast = logicalFirst + logicalLen - 1;

            uint64_t extentStart = ((uint64_t)LastExtent->ee_start_hi << 32) | (uint64_t)LastExtent->ee_start_lo;
            uint64_t physicalLast = extentStart + (logicalLast - logicalFirst);

            for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                if (!pdev->ReadSector(physicalLast * sectorsPerBlock + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                    _ds->Println("Failed to read sector");
                }
            }

            bool Inserted = false;

            /*
            * I could not figure out this part so 
            * this is written with GPT
            */
            uint16_t newTotalSize = (uint16_t)((8 + strlen(newFile) + 3) & ~3);
            DirectoryEntry2* entry = (DirectoryEntry2*)bufVirt;
            while ((uint8_t*)entry < (uint8_t*)bufVirt + blockSize) {
                if (entry->rec_len == 0) break;

                uint16_t min_len = (uint16_t)((8 + entry->name_len + 3) & ~3);
                uint16_t slack = entry->rec_len - min_len;

                if (slack >= newTotalSize) {
                    uint16_t original_total = entry->rec_len;
                    entry->rec_len = min_len;

                    DirectoryEntry2* newEntry = (DirectoryEntry2*)((uint8_t*)entry + min_len);
                    newEntry->inode = InodeNum;
                    newEntry->name_len = strlen(newFile);
                    newEntry->file_type = DirectoryEntryType::EXT4_FT_REG_FIL;
                    memcpy(newEntry->name, newFile, newEntry->name_len);
                    
                    newEntry->rec_len = original_total - min_len;

                    Inserted = true;
                    break;
                }

                entry = (DirectoryEntry2*)((uint8_t*)entry + entry->rec_len);
            }

            if (!Inserted) {
                uint32_t newParentBlock = AllocateBlock(fsN);

                if (hdr->eh_entries >= hdr->eh_max) {
                    _ds->Println("No room for new extent in inode (need tree promotion) - aborting");
                    return -2;
                }

                Extent newExt;
                newExt.ee_block = LastExtent->ee_block + LastExtent->ee_len;
                newExt.ee_len = 1;
                newExt.ee_start_lo = newParentBlock & 0xFFFFFFFF;
                newExt.ee_start_hi = (newParentBlock >> 32) & 0xFFFF;

                AddExtent(fsN, ParentInode, newExt);

                hdr->eh_entries++;

                ParentInode->i_blocks_lo += sectorsPerBlock;
                ParentInode->i_size_lo += blockSize;

                memset((void*)bufVirt, 0, blockSize);

                DirectoryEntry2* newEntry = (DirectoryEntry2*)bufVirt;
                newEntry->inode = InodeNum;
                newEntry->name_len = strlen(newFile);
                newEntry->file_type = DirectoryEntryType::EXT4_FT_REG_FIL;
                memcpy(newEntry->name, newFile, newEntry->name_len);
                newEntry->rec_len = ((8 + newEntry->name_len) + 3) & ~3u;

                for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                    if (!pdev->WriteSector(newParentBlock * sectorsPerBlock + i, (uint8_t*)bufPhys + i * pdev->SectorSize())) {
                        _ds->Println("Failed to write sector");
                    }
                }

                WriteInode(fsN->nodeId, ParentInode);

                FsNode* newfsN = (FsNode*)_ds->malloc(sizeof(FsNode));
                newfsN->atime = newInode->i_atime;
                newfsN->blocks = newInode->i_blocks_lo;
                newfsN->ctime = newInode->i_ctime;
                newfsN->gid = newInode->i_gid;
                newfsN->mode = newInode->i_mode;
                newfsN->mtime = newInode->i_mtime;
                newfsN->name = _ds->strdup(newFile);
                newfsN->nodeId = InodeNum;
                newfsN->size = 0;
                newfsN->type = FsNodeType::File;
                newfsN->uid = newInode->i_uid;
                file->node = newfsN;
                file->position = 0;

                if (buffer) {
                    if (file->flags & WR) {
                        uint64_t old = file->flags;
                        file->flags = WR | APPEND;
                        int64_t res = Write(file, buffer, size);
                        file->flags = old;
                        return res;
                    } else {
                        _ds->Println("Cannot Write to file w/o WR flag");
                        return -1;
                    }
                }
                
                return 0;
            }

            for (uint64_t i = 0; i < sectorsPerBlock; i++) {
                if (!pdev->WriteSector(physicalLast * sectorsPerBlock + i, (void*)((uint8_t*)bufPhys + i * pdev->SectorSize()))) {
                    _ds->Println("Failed to write sector");
                }
            }
            
            FsNode* newfsN = (FsNode*)_ds->malloc(sizeof(FsNode));
            newfsN->atime = newInode->i_atime;
            newfsN->blocks = newInode->i_blocks_lo;
            newfsN->ctime = newInode->i_ctime;
            newfsN->gid = newInode->i_gid;
            newfsN->mode = newInode->i_mode;
            newfsN->mtime = newInode->i_mtime;
            newfsN->name = _ds->strdup(newFile);
            newfsN->nodeId = InodeNum;
            newfsN->size = 0;
            newfsN->type = FsNodeType::File;
            newfsN->uid = newInode->i_uid;
            file->node = newfsN;
            file->position = 0;

            if (buffer) {
                if (file->flags & WR) {
                    uint64_t old = file->flags;
                    file->flags = WR | APPEND;
                    int64_t res = Write(file, buffer, size);
                    file->flags = old;
                    return res;
                } else {
                    _ds->Println("Cannot Write to file w/o WR flag");
                    return -1;
                }
            }
        }
    } else {
        _ds->Println("Unknown Flag");
        return -1;
    }
    return 0;
}

bool GenericEXT4Device::Chmod(FsNode* node, uint32_t mode) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return false;
    }
    if (readOnly) {
        _ds->Println("Can't Chmod, Read Only");
        return false;
    }
    if (!node) return false;

    Inode* ind = ReadInode(node->nodeId);

    ind->i_mode &= ~0x0FFF;
    ind->i_mode |= (mode & 0x0FFF);

    node->mode = ind->i_mode;

    WriteInode(node->nodeId, ind);

    return true;
}

bool GenericEXT4Device::Chown(FsNode* node, uint32_t uid, uint32_t gid) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return false;
    }
    if (readOnly) {
        _ds->Println("Can't Chown, Read Only");
        return false;
    }
    if (!node) return false;

    Inode* ind = ReadInode(node->nodeId);

    ind->i_uid = uid & 0xFFFF;
    ind->i_gid = gid & 0xFFFF;
    if (superblock->s_creator_os == CreatorOSIDs::Linux) {
        ind->i_osd2.linux2.l_i_uid_high = (uid >> 16) & 0xFFFF;
        ind->i_osd2.linux2.l_i_gid_high = (gid >> 16) & 0xFFFF;
    }
    if (superblock->s_creator_os == CreatorOSIDs::AstralOS) {
        ind->i_osd2.astral2.a_i_uid_high = (uid >> 16) & 0xFFFF;
        ind->i_osd2.astral2.a_i_gid_high = (gid >> 16) & 0xFFFF;
    }
    if (superblock->s_creator_os == CreatorOSIDs::GNUHurd) {
        ind->i_osd2.hurd2.h_i_uid_high = (uid >> 16) & 0xFFFF;
        ind->i_osd2.hurd2.h_i_gid_high = (gid >> 16) & 0xFFFF;
    }

    node->uid = uid;
    node->gid = gid;

    WriteInode(node->nodeId, ind);

    return true;
}

bool GenericEXT4Device::Utimes(FsNode* node, uint64_t atime, uint64_t mtime, uint64_t ctime) {
    if (!isMounted) {
        _ds->Println("FS Isnt Mounted");
        return false;
    }
    if (readOnly) {
        _ds->Println("Can't Utimes, Read Only");
        return false;
    }
    if (!node) return false;

    Inode* ind = ReadInode(node->nodeId);

    uint32_t atimelo = atime & 0xFFFFFFFF;
    uint32_t atimehi = (atime >> 32) & 0xFFFFFFFF;
    uint32_t mtimelo = mtime & 0xFFFFFFFF;
    uint32_t mtimehi = (mtime >> 32) & 0xFFFFFFFF;
    uint32_t ctimelo = ctime & 0xFFFFFFFF;
    uint32_t ctimehi = (ctime >> 32) & 0xFFFFFFFF;

    ind->i_atime = atimelo;
    ind->i_atime_extra = atimehi;
    ind->i_mtime = mtimelo;
    ind->i_mtime_extra = mtimehi;
    ind->i_ctime = ctimelo;
    ind->i_ctime_extra = ctimehi;

    node->atime = atime;
    node->mtime = mtime;
    node->ctime = ctime;

    WriteInode(node->nodeId, ind);

    return true;
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
    return "EXT4";
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
 * TODO:
 *  - Add Journal Support
*/
