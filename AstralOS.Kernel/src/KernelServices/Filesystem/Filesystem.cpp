#include "Filesystem.h"
#include "../KernelServices.h"

bool VFS::mount(const char* source, const char* target, uint64_t FSID) {
    for (int i = 0; i < mountpoints.size(); i++) {
        Mount* moun = mountpoints[i];
        if (moun->device.c_str() == source) {
            ks->basicConsole.Println("Device is already Mounted");
            return false;
        }
        if (moun->path.c_str() == target) {
            ks->basicConsole.Println("Path is in Use");
            return false;
        }
    }
    Mount* mnt = (Mount*)ks->heapAllocator.malloc(sizeof(Mount));
    mnt->device = source;
    mnt->path = target;
    mnt->FSID = FSID;
    return true;
}

Path VFS::ResolvePath(const char* pat) {
    String path = String(pat);
    Mount* best = nullptr;
    size_t bestLen = 0;

    for (int i = 0; i < mountpoints.size(); i++) {
        Mount* m = mountpoints[i];
        if (!path.StartsWith(m->path))
            continue;

        if (path.size() > m->path.size() &&
            path.c_str()[m->path.size()] != '/')
            continue;

        if (m->path.size() > bestLen) {
            best = m;
            bestLen = m->path.size();
        }
    }

    if (!best) {
        ks->basicConsole.Println("No root filesystem mounted");
    }

    String rel = "";
    if (best->path.c_str() == "/") {
        rel = path.size() > 1 ? path.substring(1) : "";
    } else {
        rel = path.substring(best->path.size());
        if (rel.StartsWith("/"))
            rel = rel.substring(1);
    }

    return {
        .path = rel,
        .FSID = best->FSID
    };
}