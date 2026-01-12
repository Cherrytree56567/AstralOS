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

struct Path {
    String path;
    uint64_t FSID;
    uint64_t disk;
    uint64_t partition;
    String device;
};

struct File {
    FsNode* node;
    uint64_t position;
    uint32_t flags;
    Path* path;
    uint64_t data;
};

enum FileFlags {
    RD = 0x01,
    WR = 0x02,
    APPEND = 0x04,
    CREATE = 0x08,
    TRUNC = 0x10
};