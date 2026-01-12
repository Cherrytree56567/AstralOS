#pragma once
#include <cstdint>
#include <cstddef>
#include "../../Utils/String/String.h"
#include "../../Utils/Array/Array.h"
#include "../DriverManager/DriverManager.h"
#include "../File/File.h"

class VFS {
public:
    bool mount(const char* source, const char* target);
    File* open(const char* path, uint32_t flags);
    bool close(File* file);
    void* read(File* file, size_t& size);
    File* mkdir(const char* path);
    File* listdir(File* file);
    bool write(File* file, void* buffer, size_t& size);
    bool chmod(File* file, uint32_t mode);
    bool chown(File* file, uint32_t uid, uint32_t gid);
    bool atime(File* file, uint64_t atime);
    bool mtime(File* file, uint64_t mtime);
    bool ctime(File* file, uint64_t ctime);
private:
    Path ResolvePath(const char* path);
    Array<Path*> mountpoints;
};
