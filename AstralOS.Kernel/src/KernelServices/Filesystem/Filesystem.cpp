#include "Filesystem.h"
#include "../KernelServices.h"

bool VFS::mount(const char* source, const char* target) {
    for (int i = 0; i < mountpoints.size(); i++) {
        Path* moun = mountpoints[i];
        String mDev = String(moun->device);
        if (mDev.c_str() == source) {
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

File* VFS::open(const char* path, uint32_t flags) {
    Path* p = (Path*)ks->heapAllocator.malloc(sizeof(Path));
    *p = ResolvePath(path);
    ks->basicConsole.Println("Creating");
    
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != p->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != p->partition) continue;
    ks->basicConsole.Println("Creating");

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;
    ks->basicConsole.Println("Creating");

        File* f = bldev->Open(p->path.c_str(), flags);
    ks->basicConsole.Println("Creating");
        f->path = p;
        f->data = 0x0;
        return f;
    }
    ks->basicConsole.Println("Failed to get File*");
    return nullptr;
}

void* VFS::read(File* file, size_t& size) {    
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        size = file->node->size;
        void* buffer = ks->heapAllocator.malloc(size);
        int64_t result = bldev->Read(file, buffer, size);
        if (result < 0) {
            ks->basicConsole.Println("Read Failed");
            return nullptr;
        }
        return buffer;
    }
    ks->basicConsole.Println("Failed to get - File*");
    return nullptr;
}

bool VFS::write(File* file, void* buffer, size_t& size) {
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        int64_t result = bldev->Write(file, buffer, size);
        if (result < 0) {
            ks->basicConsole.Println("Read Failed");
            return false;
        }
        return true;
    }
    ks->basicConsole.Println("Failed to get _ File*");
    return false;
}

bool VFS::close(File* file) {
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    ks->basicConsole.Println("F");
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        return bldev->Close(file);
    }
    if (file->data != 0x0) {
        FsNode** LFSN = (FsNode**)file->data;
        ks->basicConsole.Println(to_hstring(file->node->size));
        for (uint64_t i = 0; i < file->node->size; i++) {
            FsNode* CLFsN = LFSN[i];
            free(CLFsN);
        }
        free(LFSN);
    }
    ks->basicConsole.Println("Failed to get File*");
    return false;
}

File* VFS::mkdir(const char* path) {
    Path resPath = ResolvePath(path);
    size_t size = 0;
    char** p = str_split(strdup(resPath.path.c_str()), '/', &size);

    uint64_t newSize = 0;
    for (size_t i = 0; i < (size - 1); i++) {
        if ((size - 1) == 1) {
            newSize += strlen(p[i - 1]);
        } else {
            newSize += strlen(p[i - 1]) + 1;
        }
    }

    char* newPath = (char*)ks->heapAllocator.malloc(newSize);
    
    for (size_t i = 0; i < (size - 1); i++) {
        if ((size - 1) == 1) {
            strcat(newPath, p[i]);
        } else {
            strcat(newPath, p[i]);
            strcat(newPath, "/");
        }
    }

    char* newDir = p[size - 1];

    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != resPath.disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != resPath.partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        FsNode* fsN = bldev->FindDir(bldev->GetParentLayer()->GetMountNode(), newPath);

        File* file = (File*)ks->heapAllocator.malloc(sizeof(File));

        FsNode* CfsN = bldev->CreateDir(fsN, newDir);
        if (!CfsN) {
            ks->basicConsole.Println("Failed to Create dir");
            return nullptr;
        }

        Path* rPath = (Path*)ks->heapAllocator.malloc(sizeof(Path));
        rPath->device = resPath.device;
        rPath->disk = resPath.disk;
        rPath->FSID = resPath.FSID;
        rPath->partition = resPath.partition;
        rPath->path = resPath.path;

        file->flags = RD;
        file->node = CfsN;
        file->path = rPath;
        file->position = 0;
        file->data = 0;
        return file;
    }

    return nullptr;
}

File* VFS::listdir(File* file) {
    if (file->data == 0x0) {
        Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
        for (size_t i = 0; i < FSDriver.size(); i++) {
            FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

            if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
            if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

            if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

            size_t count = 0;
            FsNode** LfsN = bldev->ListDir(file->node, &count);

            file->node->size = count;
            file->data = (uint64_t)LfsN;
            file->position = 0;

            char* newDirName = (char*)ks->heapAllocator.malloc(file->path->path.size() + String(LfsN[file->position]->name).size() + 2);
            newDirName[0] = '\0';
            strcat(newDirName, file->path->path.c_str());
            strcat(newDirName, "/");
            strcat(newDirName, LfsN[file->position]->name);

            File* newDir = (File*)ks->heapAllocator.malloc(sizeof(File));
            newDir->data = 0;
            newDir->flags = file->flags;
            newDir->node = LfsN[file->position];
            newDir->position = 0;
            
            Path* newDirPath = (Path*)ks->heapAllocator.malloc(sizeof(Path));
            newDirPath->device = file->path->device;
            newDirPath->disk = file->path->disk;
            newDirPath->FSID = file->path->FSID;
            newDirPath->partition = file->path->partition;
            newDirPath->path = newDirName;

            newDir->path = newDirPath;

            file->position++;

            return newDir;
        }
    } else {
        if (file->position > file->node->size) {
            return nullptr;
        }
        FsNode** LfsN = (FsNode**)file->data;
        char* newDirName = (char*)ks->heapAllocator.malloc(file->path->path.size() + String(LfsN[file->position]->name).size() + 2);
        newDirName[0] = '\0';
        strcat(newDirName, file->path->path.c_str());
        strcat(newDirName, "/");
        strcat(newDirName, LfsN[file->position]->name);

        File* newDir = (File*)ks->heapAllocator.malloc(sizeof(File));
        newDir->data = 0;
        newDir->flags = file->flags;
        newDir->node = LfsN[file->position];
        newDir->position = 0;
            
        Path* newDirPath = (Path*)ks->heapAllocator.malloc(sizeof(Path));
        newDirPath->device = file->path->device;
        newDirPath->disk = file->path->disk;
        newDirPath->FSID = file->path->FSID;
        newDirPath->partition = file->path->partition;
        newDirPath->path = newDirName;

        newDir->path = newDirPath;

        file->position++;

        return newDir;
    }
}

bool VFS::chmod(File* file, uint32_t mode) {
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        return bldev->Chmod(file->node, mode);
    }
    ks->basicConsole.Println("Failed to get File*");
    return false;
}

bool VFS::chown(File* file, uint32_t uid, uint32_t gid) {    
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        return bldev->Chown(file->node, uid, gid);
    }
    ks->basicConsole.Println("Failed to get File*");
    return false;
}

bool VFS::atime(File* file, uint64_t atime) {
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        return bldev->Utimes(file->node, atime, file->node->mtime, file->node->ctime);
    }
    ks->basicConsole.Println("Failed to get File*");
    return false;
}

bool VFS::mtime(File* file, uint64_t mtime) {
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        return bldev->Utimes(file->node, file->node->atime, mtime, file->node->ctime);
    }
    ks->basicConsole.Println("Failed to get File*");
    return false;
}

bool VFS::ctime(File* file, uint64_t ctime) {
    Array<BaseDriver*> FSDriver = ks->driverMan.GetDevices(DriverType::FilesystemDriver);
    for (size_t i = 0; i < FSDriver.size(); i++) {
        FilesystemDevice* bldev = static_cast<FilesystemDevice*>(FSDriver[i]);

        if (bldev->GetParentLayer()->GetParentLayer()->GetDrive() != file->path->disk) continue;
        if (bldev->GetParentLayer()->GetPartition() != file->path->partition) continue;

        if (bldev->GetParentLayer()->SectorCount() == 0 || bldev->GetParentLayer()->SectorSize() == 0) continue;

        return bldev->Utimes(file->node, file->node->atime, file->node->mtime, ctime);
    }
    ks->basicConsole.Println("Failed to get File*");
    return false;
}