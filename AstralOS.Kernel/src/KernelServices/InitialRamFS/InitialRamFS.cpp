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
    size_t count = 0;
    char* pathCopy = strdup(origPath); // Copy the original path to avoid modifying it
    char* token;
    char* delim = "/";  // Delimiter for splitting the path

    // Split by "/"
    token = strtok(pathCopy, delim);
    while (token != NULL && count < maxParts) {
        parts[count++] = strdup(token); // Store the token (path part)
        token = strtok(NULL, delim);    // Continue splitting
    }

    free(pathCopy); // Free the copy of the original path
    return count;   // Return the number of parts
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

Array<char*> InitialRamFS::list(char* dir) {
    Array<char*> output;
    uint64_t ptr = (uint64_t)base;

    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;

        if (memcmp(header->magic, "070701", 6) != 0) {
            ks->basicConsole.Println("Invalid CPIO magic number!");
            break;
        }

        uint32_t nameSize = parse_hex(header->namesize, 8);
        uint32_t fileSize = parse_hex(header->filesize, 8);

        char* filenameBuf = (char*)(ptr + 110);
        if (!filenameBuf) {
            ks->basicConsole.Println("Out of memory!");
            break;
        }
        filenameBuf[nameSize - 1] = '\0';

        uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
        uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
        uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

        if (strcmp(filenameBuf, "TRAILER!!!") == 0) {
            break;
        }

        if (strcmp(filenameBuf, ".") == 0) {
            ptr = next_ptr;
            continue;
        }

        char* parts[10];
        size_t partSize = split_path(filenameBuf, parts, 10);
        char* Dirparts[10];
        size_t DirpartSize = split_path(dir, Dirparts, 10);

        bool isFound = true;

        for (size_t i = 0; i < DirpartSize; i++) {
            if (strcmp(parts[i], Dirparts[i]) != 0) {
                isFound = false;
            }
        }

        if (isFound && partSize == DirpartSize + 1) {
            output.push_back(parts[partSize - 1]);
        }

        ptr = next_ptr;
    }
    return output;
}

void* InitialRamFS::read(char* dir, char* name) {
    uint64_t ptr = (uint64_t)base;

    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;

        if (memcmp(header->magic, "070701", 6) != 0) {
            ks->basicConsole.Println("Invalid CPIO magic number!");
            break;
        }

        uint32_t nameSize = parse_hex(header->namesize, 8);
        uint32_t fileSize = parse_hex(header->filesize, 8);

        uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
        uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
        uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

        char* filenameBuf = (char*)(ptr + 110);
        if (!filenameBuf) {
            ks->basicConsole.Println("Out of memory!");
            break;
        }
        filenameBuf[nameSize - 1] = '\0';

        if (strcmp(filenameBuf, "TRAILER!!!") == 0) {
            break;
        }

        if (strcmp(filenameBuf, ".") == 0) {
            ptr = next_ptr;
            continue;
        }

        char* parts[10];
        size_t partSize = split_path(filenameBuf, parts, 10);
        char* Dirparts[10];
        size_t DirpartSize = split_path(dir, Dirparts, 10);

        if (partSize > (DirpartSize + 1)) {
            for (int i = 0; i < partSize; i++) {
                free(parts[i]);
            }
            for (int i = 0; i < DirpartSize; i++) {
                free(Dirparts[i]);
            }
            ptr = next_ptr;
            continue;
        }

        bool isFound = true;

        for (size_t i = 0; i < DirpartSize; i++) {
            if (i == DirpartSize) {
                break;
            }
            if (strcmp(parts[i], Dirparts[i]) != 0) {
                isFound = false;
            }
        }

        if (isFound) {
            if (strcmp(parts[partSize], name) != 0){
                void* buffer = malloc(fileSize);
                if (!buffer) {
                    ks->basicConsole.Println("malloc failed!");
                    return nullptr;
                }
                memcpy(buffer, (void*)file_ptr, fileSize);\
                return buffer;
            }
        }

        ptr = next_ptr;
    }
}