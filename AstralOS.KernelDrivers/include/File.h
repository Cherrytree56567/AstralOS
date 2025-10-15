#pragma once
#include <cstdint>

enum class FsNodeType {
    File,
    Directory,
    Symlink,
    Device,
    Unknown
};

struct FsNode {
    FsNodeType type;
    char* name;
    uint64_t nodeId;
    uint64_t size;
    uint64_t blocks;

    uint32_t mode;
    uint32_t uid;
    uint32_t gid;

    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
};

struct File {
    FsNode* node;
    uint64_t position;
    uint32_t flags;
};