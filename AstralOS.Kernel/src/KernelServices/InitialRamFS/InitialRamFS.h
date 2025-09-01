#pragma once
#include <cstdint>
#include <cstddef>
#include "../../Utils/cpu.h"
#include "../../Utils/utils.h"
#include "../../Utils/Array/Array.h"

struct CPIOHeader {
    char magic[6];
    char ino[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8];
} __attribute__((packed));

class InitialRamFS {
public:
    InitialRamFS() {}

    void Initialize(void* base, uint64_t size);

    bool file_exists(char* name);
    bool dir_exists(char* name);

    void* read(char* dir, char* name, size_t* outSize);

    Array<char*> list(char* dir);
private:    
    void* base;
    uint64_t size;
};