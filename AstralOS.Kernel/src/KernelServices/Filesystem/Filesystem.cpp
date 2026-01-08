#include "Filesystem.h"
#include "../KernelServices.h"

bool VFS::mount(const char* source, const char* target) {
    for (int i = 0; i < mountpoints.size(); i++) {
        Path* moun = mountpoints[i];
        if (moun->device.c_str() == source) {
            ks->basicConsole.Println("Device is already Mounted");
            return false;
        }
        if (moun->path.c_str() == target) {
            ks->basicConsole.Println("Path is in Use");
            return false;
        }
    }
    Path* mnt = (Path*)ks->heapAllocator.malloc(sizeof(Path));
    mnt->device = source;
    mnt->path = target;

    uint64_t size = 0;
    char** devs = str_split(strdup((char*)mnt->device.c_str()), '/', &size);
    if (size != 3) {
        ks->basicConsole.Print("Dev path doesn't have 3 values: ");
        ks->basicConsole.Println(to_hstring(size));
        return false;
    }

    if (strcmp(devs[0], "dev") != 0) {
        ks->basicConsole.Println("Dev path doesn't start with /dev");
        return false;
    }
    uint64_t diskPathSize = 0;
    char** diskPath = str_split(strdup(devs[1]), 'p', &diskPathSize);
    if (diskPathSize == 0) {
        ks->basicConsole.Println("Failed to get DiskPath Split");
        return false;
    }
    char* name = strdup(diskPath[0]);

    int idx = 0;

    /*
     * Skip the `sd` part
    */
    name += 2;

    while (*name) {
        idx = idx * 26 + (*name - 'a' + 1);
        name++;
    }

    uint64_t num = idx - 1;

    uint64_t partition = 0;

    if (diskPathSize == 2) {
        int value = 0;

        for (int i = 0; diskPath[1][i]; i++) {
            if (diskPath[1][i] < '0' || diskPath[1][i] > '9') {
                ks->basicConsole.Println("Failed to get Partition Num");
                break;
            }

            value = value * 10 + (diskPath[1][i] - '0');
        }
        partition = value;
    }

    bool mounted = false;

    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != num) continue;
        if (bldev->GetParentLayer()->GetPartition() != partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;
        if (bldev->Support()) {
            FsNode* fsN = bldev->Mount();            
            mnt->FSID = bldev->GetParentLayer()->GetMount();
            mnt->disk = num;
            mnt->partition = partition;
            mountpoints.push_back(mnt);
            mounted = true;
            ks->basicConsole.Println(((String)"Mounted disk " + to_hstring(num) + 
                                              ", partition " + to_hstring(partition) + 
                                              " at " + mnt->path).c_str());
        }
    }
    return mounted;
}

Path VFS::ResolvePath(const char* pat) {
    String path = String(pat);
    Path* best = nullptr;
    size_t bestLen = 0;

    for (int i = 0; i < mountpoints.size(); i++) {
        Path* m = mountpoints[i];
        if (!path.StartsWith(m->path))
            continue;
        
        if (strcmp(m->path.c_str(), "/") != 0) {
            if (path.size() > m->path.size() && path.c_str()[m->path.size()] != '/') {
                continue;
            }
        }

        if (m->path.size() > bestLen) {
            best = m;
            bestLen = m->path.size();
        }
    }

    if (!best) {
        ks->basicConsole.Println("No root filesystem mounted");
        return Path{
            .path = "",
            .FSID = 0,
            .disk = 0,
            .partition = 0,
            .device = ""
        };
    }

    String rel = "";
    if (strcmp(best->path.c_str(), "/") == 0) {
        rel = path.size() > 1 ? path.substring(1) : "";
    } else {
        rel = path.substring(best->path.size());
        if (rel.StartsWith("/"))
            rel = rel.substring(1);
    }

    return {
        .path = rel,
        .FSID = best->FSID,
        .disk = best->disk,
        .partition = best->partition,
        .device = best->device
    };
}

File* VFS::open(const char* path, FileFlags flags) {
    File* file = (File*)ks->heapAllocator.malloc(sizeof(File));
    Path p = ResolvePath(path);
    
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != p.disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != p.partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        File* f = bldev->Open(p.path.c_str(), static_cast<uint32_t>(flags));
        f->path = p;
        return f;
    }
    ks->basicConsole.Println("Failed to get File*");
    return nullptr;
}

void* VFS::read(File* file, size_t& size) {
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        ks->basicConsole.Println(((String)"Found FS Driver: " + FSDriver[i]->DriverName()).c_str());
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path.disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path.partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        uint64_t size = bldev->Read(file, static_cast<uint32_t>());
    }
    ks->basicConsole.Println("Failed to get File*");
    return nullptr;
}