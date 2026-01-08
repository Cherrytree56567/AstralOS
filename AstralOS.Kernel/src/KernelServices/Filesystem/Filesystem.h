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
    File* open(const char* path, FileFlags flags);
    bool close(File* file);
    void* read(File* file, size_t& size);
    bool write(File* file, void* buffer, size_t& size);
private:
    Path ResolvePath(const char* path);
    Array<Path*> mountpoints;
};
