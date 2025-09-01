#include "InitialRamFS.h"
#include "../KernelServices.h"

/*
 * This function is generated from ChatGPT.
*/
static uint32_t parse_hex(const char* str, size_t len = 8) {
    uint32_t result = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = str[i];
        result <<= 4;
        if      (c >= '0' && c <= '9') result |= (c - '0');
        else if (c >= 'A' && c <= 'F') result |= (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') result |= (c - 'a' + 10);
    }
    return result;
}

/*
 * This function is generated from ChatGPT.
*/
size_t split_path(const char* origPath, char* parts[], size_t maxParts) {
    static char buffer[256];
    strncpy(buffer, origPath, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* path = buffer;
    size_t count = 0;

    while (*path && count < maxParts) {
        while (*path == '/') path++;
        if (!*path) break;

        parts[count++] = path;

        while (*path && *path != '/') path++;

        if (*path == '/') {
            *path = '\0';
            path++;
        }
    }

    return count;
}

/*
 * BTW, these functions aren't generated with GPT.
 * 
 * Initialize decodes the CPIO and adds the 
 * data to a struct to easily read it.
 * 
 * First we should get the CPIOHeader and check
 * it's magic number.
 * 
 * Then we can get the size of the name and file
 * to parse the name and data.
 * 
 * If the name is `TRAILER!!!` we can stop parsing
 * because that marks the end of the CPIO archive.
 * 
 * We can then check the mode of the entry to see
 * if it is a directory or a file.
*/
void InitialRamFS::Initialize(void* bas, uint64_t siz) {
    base = bas;
    size = siz;
}

/* 
 * We can then split `name` (which includes the 
 * path) and check if both parts are the same.
*/
bool InitialRamFS::file_exists(char* name) {
    uint64_t ptr = (uint64_t)base;
    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;

        if (memcmp(header->magic, "070701", 6) != 0) {
            ks->basicConsole.Println("Invalid CPIO magic number!");
            break;
        }

        uint32_t nameSize = parse_hex(header->namesize, 8);
        uint32_t fileSize = parse_hex(header->filesize, 8);

        const char* filename = (const char*)(ptr + sizeof(CPIOHeader));

        if (strcmp(filename, "TRAILER!!!") == 0) {
            break;
        }

        if (strcmp(filename, ".") == 0) {
            uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
            uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
            uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

            ptr = next_ptr;
            continue;
        }

        uint32_t mode = parse_hex(header->mode, 8); 
        bool is_dir = (mode & 0xF000) == 0x4000;

        if (!is_dir && strcmp(filename, name) == 0) {
            return true;
        }

        uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
        uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
        uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

        ptr = next_ptr;
    }
    
    return false;
}

bool InitialRamFS::dir_exists(char* name) {
    uint64_t ptr = (uint64_t)base;
    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;

        if (memcmp(header->magic, "070701", 6) != 0) {
            ks->basicConsole.Println("Invalid CPIO magic number!");
            break;
        }

        uint32_t nameSize = parse_hex(header->namesize, 8);
        uint32_t fileSize = parse_hex(header->filesize, 8);

        const char* filename = (const char*)(ptr + sizeof(CPIOHeader));

        if (strcmp(filename, "TRAILER!!!") == 0) {
            break;
        }

        if (strcmp(filename, ".") == 0) {
            uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
            uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
            uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

            ptr = next_ptr;
            continue;
        }

        uint32_t mode = parse_hex(header->mode, 8); 
        bool is_dir = (mode & 0xF000) == 0x4000;

        if (is_dir && strcmp(filename, name) == 0) {
            return true;
        }

        uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
        uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
        uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

        ptr = next_ptr;
    }
    
    return false;
}

void* InitialRamFS::read(char* dir, char* name, size_t* outSize) {
    return nullptr;
}

Array<char[512]> InitialRamFS::list(char* dir) {
    Array<char[512]> output;
    uint64_t ptr = (uint64_t)base;

    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;

        if (memcmp(header->magic, "070701", 6) != 0) {
            ks->basicConsole.Println("Invalid CPIO magic number!");
            break;
        }

        uint32_t nameSize = parse_hex(header->namesize, 8);
        uint32_t fileSize = parse_hex(header->filesize, 8);

        ks->basicConsole.Println("BEF .");

        char filenameBuf[512];
        if (nameSize >= sizeof(filenameBuf)) {
            ks->basicConsole.Println("Filename too long!");
            break;
        }
        memcpy(filenameBuf, (const void*)(ptr + sizeof(CPIOHeader)), nameSize);
        filenameBuf[nameSize] = '\0';

        if (strcmp(filenameBuf, "TRAILER!!!") == 0) {
            break;
        }

        if (strcmp(filenameBuf, ".") == 0) {
            continue;
        }

        ks->basicConsole.Println("AFTER .");

        char* parts[10];
        size_t partSize = split_path(filenameBuf, parts, 10);
        char* Dirparts[10];
        size_t DirpartSize = split_path(dir, Dirparts, 10);

        if (partSize > (DirpartSize + 1)) {
            ks->basicConsole.Print("Skipping: ");
            ks->basicConsole.Println(filenameBuf);
            ks->basicConsole.Print("DirPart: ");
            ks->basicConsole.Println(to_hstring(DirpartSize));
            ks->basicConsole.Print("Part: ");
            ks->basicConsole.Println(to_hstring(partSize));
            uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
            uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
            uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

            ptr = next_ptr;
            continue;
        }

        bool isFound = true;

        ks->basicConsole.Print("FileNAME: ");
        ks->basicConsole.Println(filenameBuf);

        for (size_t i = 0; i < DirpartSize; i++) {
            ks->basicConsole.Print("-");
            if (strcmp(parts[i], Dirparts[i]) != 0) {
                isFound = false;
            }
        }
        
        ks->basicConsole.Println("END");

        if (isFound) {
            ks->basicConsole.Print("Adding: ");
            ks->basicConsole.Println(filenameBuf);
            output.push_back(filenameBuf);
        }

        uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
        uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
        uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

        ptr = next_ptr;
    }
    return output;
}