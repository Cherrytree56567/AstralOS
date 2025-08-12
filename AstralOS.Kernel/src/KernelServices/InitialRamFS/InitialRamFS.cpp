#include "InitialRamFS.h"
#include "../KernelServices.h"

/*
 * This code is generated from ChatGPT.
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

    return count - 1;
}

void PrintDirectory(const DirectoryEntry& dir, int depth = 0) {
    for (int i = 0; i < depth; ++i) {
        ks->basicConsole.Print("  ");
    }

    ks->basicConsole.Print("[DIR] ");
    ks->basicConsole.Println(dir.name);

    for (size_t i = 0; i < dir.files.size; ++i) {
        for (int j = 0; j < depth + 1; ++j) {
            ks->basicConsole.Print("  ");
        }
        ks->basicConsole.Print("- ");
        ks->basicConsole.Println(dir.files[i + 1].name);
    }

    for (size_t i = 0; i < dir.subdirs.size; ++i) {
        PrintDirectory(dir.subdirs[i + 1], depth + 1);
    }
}

/*
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
    uint64_t ptr = (uint64_t)base;
    root.files.clear();
    root.subdirs.clear();
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

        char* parts[10];
        size_t count = split_path((char*)filename, parts, 10);

        DirectoryEntry* currentDir = &root;
        
        for (size_t i = 0; i < (count + 1); ++i) {
            if (currentDir == nullptr) {
                ks->basicConsole.Println("Current directory is null!");
                break;
            }

            if (i == count) {
                if (is_dir) {
                    DirectoryEntry newDir;
                    newDir.files.clear();
                    newDir.subdirs.clear();
                    newDir.name = parts[i];
                    currentDir->subdirs.push_back(newDir);
                    continue;
                } else {
                    FileEntry newFile;
                    newFile.name = parts[i];
                    newFile.data = (void*)(ptr + sizeof(CPIOHeader) + nameSize);
                    newFile.size = fileSize;
                    currentDir->files.push_back(newFile);
                    continue;
                }
            }

            for (size_t j = 0; j < currentDir->subdirs.size; ++j) {
                if (strcmp(currentDir->subdirs[j + 1].name, parts[i]) == 0) {
                    currentDir = &currentDir->subdirs[j + 1];
                    break;
                }
            }
        }
        uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
        uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
        uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

        ptr = next_ptr;
    }
    for (size_t i = 0; i < root.subdirs.size; i++) {
        PrintDirectory(root.subdirs[i + 1]);
    }
}

bool InitialRamFS::file_exists(char* name) {
}

bool InitialRamFS::dir_exists(char* name) {
}

void* InitialRamFS::read(char* dir, char* name, size_t* outSize) {
}

Array<const char*> InitialRamFS::list(char* dir) {
    Array<const char*> result;
    DirectoryEntry currentDir = root;

    char* parts[10];
    size_t count = split_path((char*)dir, parts, 10);

    for (size_t i = 0; i < root.subdirs.size; i++) {
        PrintDirectory(root.subdirs[i + 1]);
    }

    for (size_t i = 0; i < count; ++i) {
        if (currentDir.subdirs.size == 0 && currentDir.files.size == 0) {
            ks->basicConsole.Println("Current directory is null!");
            break;
        }

        if (strcmp(parts[i], ".") == 0) {
            continue;
        }

        for (size_t j = 0; j < currentDir.subdirs.size; ++j) {
            ks->basicConsole.Println(currentDir.subdirs[j + 1].name);
            if (strcmp(currentDir.subdirs[j + 1].name, parts[i]) == 0) {
                currentDir = currentDir.subdirs[j + 1];
                ks->basicConsole.Print("ADir: ");
                ks->basicConsole.Println(currentDir.name);
                break;
            }
        }
    }

    for (size_t j = 0; j < currentDir.subdirs.size; ++j) {
        result.push_back(currentDir.subdirs[j + 1].name);
        ks->basicConsole.Print("Dir: ");
        ks->basicConsole.Println(currentDir.subdirs[j + 1].name);
    }
    for (size_t j = 0; j < currentDir.files.size; ++j) {
        result.push_back(currentDir.files[j + 1].name);
        ks->basicConsole.Print("File: ");
        ks->basicConsole.Println(currentDir.files[j + 1].name);
    }
    return result;
}