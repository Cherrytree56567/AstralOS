#include "InitialRamFS.h"

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

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;

    for (size_t i = 0; i < n; ++i) {
        if (p1[i] != p2[i])
            return p1[i] - p2[i];
    }
    return 0;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];

    return dest;
}

char* strncpy(char* dest, const char* src, size_t maxLen) {
    if (maxLen == 0) return dest;

    size_t i = 0;
    while (i < maxLen - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
    return dest;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

char* strchr(const char* str, int ch) {
    while (*str) {
        if (*str == (char)ch) {
            return (char*)str;
        }
        ++str;
    }
    return nullptr;
}

void InitialRamFS::Initialize(void* bas, uint64_t siz) {
    base = bas;
    size = siz;
}

bool InitialRamFS::file_exists(char* name) {
    uint8_t* ptr = (uint8_t*)base;

    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;
        if (memcmp(header->magic, "070701", 6) != 0) break;

        uint32_t namesize = parse_hex(header->namesize);
        char* filename = (char*)(ptr + sizeof(CPIOHeader));
        if (strcmp(filename, "TRAILER!!!") == 0) break;

        if (strcmp(filename, name) == 0) return true;

        uint32_t filesize = parse_hex(header->filesize);
        uintptr_t next = (uintptr_t)filename + namesize;
        next = (next + 3) & ~3;
        next += filesize;
        next = (next + 3) & ~3;
        ptr = (uint8_t*)next;
    }
    return false;
}

bool InitialRamFS::dir_exists(char* name) {
    size_t name_len = strlen(name);
    if (name[name_len - 1] != '/')
        name_len++;

    uint8_t* ptr = (uint8_t*)base;

    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;
        if (memcmp(header->magic, "070701", 6) != 0) break;

        uint32_t namesize = parse_hex(header->namesize);
        char* filename = (char*)(ptr + sizeof(CPIOHeader));
        if (strcmp(filename, "TRAILER!!!") == 0) break;

        if (strncmp(filename, name, name_len - 1) == 0 && filename[name_len - 1] == '/') return true;

        uint32_t filesize = parse_hex(header->filesize);
        uintptr_t next = (uintptr_t)filename + namesize;
        next = (next + 3) & ~3;
        next += filesize;
        next = (next + 3) & ~3;
        ptr = (uint8_t*)next;
    }
    return false;
}

void* InitialRamFS::read(char* dir, char* name, size_t* outSize) {
    char fullpath[256];
    int i = 0;

    while (dir[i] && i < 255) {
        fullpath[i] = dir[i];
        i++;
    }

    if (i > 0 && fullpath[i - 1] != '/' && i < 255) {
        fullpath[i++] = '/';
    }

    int j = 0;
    while (name[j] && i < 255) {
        fullpath[i++] = name[j++];
    }

    fullpath[i] = '\0';

    uint8_t* ptr = (uint8_t*)base;

    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;
        if (memcmp(header->magic, "070701", 6) != 0) break;

        uint32_t namesize = parse_hex(header->namesize);
        uint32_t filesize = parse_hex(header->filesize);
        char* filename = (char*)(ptr + sizeof(CPIOHeader));
        if (strcmp(filename, "TRAILER!!!") == 0) break;

        if (strcmp(filename, fullpath) == 0) {
            uintptr_t data_ptr = (uintptr_t)filename + namesize;
            data_ptr = (data_ptr + 3) & ~3;
            if (outSize) *outSize = filesize;
            return (void*)data_ptr;
        }

        uintptr_t next = (uintptr_t)filename + namesize;
        next = (next + 3) & ~3;
        next += filesize;
        next = (next + 3) & ~3;
        ptr = (uint8_t*)next;
    }

    if (outSize) *outSize = 0;
    return nullptr;
}

char* InitialRamFS::list(char* dir) {
    static char buffer[2048];
    char* out = buffer;

    char fullpath[256];
    strncpy(fullpath, dir, sizeof(fullpath) - 2);
    size_t dir_len = strlen(fullpath);
    if (dir_len > 0 && fullpath[dir_len - 1] != '/') {
        fullpath[dir_len++] = '/';
        fullpath[dir_len] = '\0';
    }

    uint8_t* ptr = (uint8_t*)base;

    while (true) {
        CPIOHeader* header = (CPIOHeader*)ptr;
        if (memcmp(header->magic, "070701", 6) != 0) break;

        uint32_t namesize = parse_hex(header->namesize);
        uint32_t filesize = parse_hex(header->filesize);
        char* filename = (char*)(ptr + sizeof(CPIOHeader));

        if (strcmp(filename, "TRAILER!!!") == 0) break;

        bool match = false;
        const char* name_after = nullptr;

        if (dir_len == 0) {
            if (!strchr(filename, '/')) {
                match = true;
                name_after = filename;
            }
        } else if (strncmp(filename, fullpath, dir_len) == 0) {
            name_after = filename + dir_len;
            if (!strchr(name_after, '/')) {
                match = true;
            }
        }

        if (match && name_after) {
            size_t len = strlen(name_after);
            if ((out - buffer) + len + 2 < sizeof(buffer)) {
                memcpy(out, name_after, len);
                out[len] = '\n';
                out[len + 1] = '\0';
                out += len + 1;
            }
        }

        uintptr_t name_ptr = (uintptr_t)(ptr + sizeof(CPIOHeader));
        uintptr_t file_ptr = (name_ptr + namesize + 3) & ~3;
        uintptr_t next = (file_ptr + filesize + 3) & ~3;
        ptr = (uint8_t*)next;
    }

    *out = '\0';
    return buffer;
}