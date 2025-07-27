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

void PrintDirectory(const DirectoryEntry& dir, int depth = 0) {
    // Indent based on depth
    for (int i = 0; i < depth; ++i)
        ks->basicConsole.Print("  ");

    ks->basicConsole.Print(" ^ ");
    ks->basicConsole.Println(dir.name ? dir.name : "(root)");

    // List files
    for (size_t i = 0; i < dir.files.size; ++i) {
        const FileEntry& file = dir.files[i];

        for (int j = 0; j < depth + 1; ++j)
            Print("  ");

        ks->basicConsole.Print(" - ");
        ks->basicConsole.Print(file.name);
        ks->basicConsole.Print(" (");
        ks->basicConsole.Print(to_hstring(file.size));
        ks->basicConsole.Println(" bytes)");
    }

    // Recurse into subdirs
    for (size_t i = 0; i < dir.subdirs.size; ++i) {
        PrintDirectory(dir.subdirs[i], depth + 1);
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
    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;

        if (memcmp(header->magic, "070701", 6) != 0) {
            ks->basicConsole.Println("Invalid CPIO magic number!");
            break;
        }

        ks->basicConsole.Print("Fo");

        uint32_t nameSize = parse_hex(header->namesize, 8);
        uint32_t fileSize = parse_hex(header->filesize, 8);

        const char* filename = (const char*)(ptr + sizeof(CPIOHeader));

        if (strcmp(filename, "TRAILER!!!") == 0) {
            break;
        }

        if (strcmp(filename, ".") == 0) {
            uintptr_t name_ptr = ptr + sizeof(CPIOHeader); // right after header
            uintptr_t filename_end = name_ptr + nameSize;

            // Align filename_end to next 4-byte boundary
            uintptr_t file_ptr = (filename_end + 3) & ~3;

            // Align file end to next 4-byte boundary
            uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

            ptr = next_ptr;
            continue;
        }

        uint32_t mode = parse_hex(header->mode, 8);
        bool is_dir = (mode & 0xF000) == 0x4000;

        size_t count = 0;
        char* parts[10];
        char* path = (char*)filename;

        while (*path) {
            while (*path == '/') path++;
            if (!*path) break;

            parts[count++] = path;
            while (*path && *path != '/') path++;

            if (*path == '/') {
                *(char*)path = '\0';
                path++;
            }
        }
            
        char* _name = parts[count - 1];

        DirectoryEntry* current = &root;

        ks->basicConsole.Print("Processing: ");

        for (size_t i = 0; i < (count-1); ++i) {
            if (!current) {
                ks->basicConsole.Println("ERROR: current is null!");
                break;
            }
ks->basicConsole.Print("Processisng: ");
            const char* seg = parts[i];
            if (i == count - 1) {
                if (is_dir) {
                    DirectoryEntry newDir;
                    newDir.name = seg;
                    current->subdirs.push_back(newDir);
                    ks->basicConsole.Print("Created directory: ");
                    ks->basicConsole.Println(seg);
                } else {
                    FileEntry fileEntry;
                    fileEntry.name = seg;
                    uintptr_t name_ptr = ptr + sizeof(CPIOHeader);
                    uintptr_t file_ptr = (name_ptr + nameSize + 3) & ~3;
                    fileEntry.data = (void*)file_ptr; // Make sure you decode the hex
                    fileEntry.size = fileSize;
                    current->files.push_back(fileEntry);
                    ks->basicConsole.Print("Created file: ");
                    ks->basicConsole.Println(seg);
                    ks->basicConsole.Print("File size: ");
                    ks->basicConsole.Println(to_hstring(fileSize));
                }
                break;
            }

            bool found = false;
            for (size_t j = 0; j < current->subdirs.size; ++j) {
                ks->basicConsole.Print("Checking subdir: ");
                ks->basicConsole.Println(current->subdirs[j].name);
                if (strcmp(current->subdirs[j].name, seg) == 0) {
                    current = &current->subdirs[j];
                    found = true;
                    break;
                }
            }

            if (!found) {
                DirectoryEntry newDir;
                newDir.name = seg;
                ks->basicConsole.Print("Creating new directory: ");
                ks->basicConsole.Println(seg);
                ks->basicConsole.Print("Current directory: ");
                ks->basicConsole.Println(current->name ? current->name : "(root)");
                ks->basicConsole.Print("Current subdirs count: ");
                ks->basicConsole.Println(to_hstring(current->subdirs.size));
                current->subdirs.push_back(newDir);
                current = &current->subdirs[current->subdirs.size - 1];
            }
        }
        uintptr_t name_ptr = ptr + sizeof(CPIOHeader); // right after header
        uintptr_t filename_end = name_ptr + nameSize;

        // Align filename_end to next 4-byte boundary
        uintptr_t file_ptr = (filename_end + 3) & ~3;

        // Align file end to next 4-byte boundary
        uintptr_t next_ptr = (file_ptr + fileSize + 3) & ~3;

        ptr = next_ptr;
    }
    PrintDirectory(root);
}

bool InitialRamFS::file_exists(char* name) {
}

bool InitialRamFS::dir_exists(char* name) {
}

void* InitialRamFS::read(char* dir, char* name, size_t* outSize) {
}

Array<const char*> InitialRamFS::list(char* dir) {
    
}