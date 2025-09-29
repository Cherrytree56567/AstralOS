#include "main.h"
#include "elf.h"
#include "Utils.h"
#include "paging.h"
#include "pfa.h"

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
	gImageHandle = ImageHandle;
    ggST = SystemTable;

    EFI_MEMORY_DESCRIPTOR* Map = NULL;
    uint64_t MapSize, MapKey;
    uint64_t DescriptorSize;
    uint32_t DescriptorVersion;

    MapSize = 0;
    EFI_STATUS Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);

    MapSize += DescriptorSize;

    Status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, MapSize, (void**)&Map);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to allocate memory map: %r\n", Status);
        return Status;
    }

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get memory map: %r\n", Status);
        return Status;
    }

    PageFrameAllocator pageframealloc;
    pageframealloc.initialized = 0;
    PFA_ReadEFIMemoryMap(&pageframealloc, Map, MapSize, DescriptorSize);

    InitialisePaging();
    Print(L"Paging initialised.\n");
    /*
     * We need to get the RSDP
    */
    void* rsdsp = FindRSDP(SystemTable);
    if (rsdsp == NULL) {
        Print(L"RSDP not found\n");
        return EFI_NOT_FOUND;
    }


    /*
     * Most of the code here is from pbatard/uefi-ntfs
     * https://github.com/pbatard/uefi-ntfs
    */
    int64_t SecureBootStatus = GetSecureBootStatus();

    /*
	* Get Current Boot Image to get the device path
	*/
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	Status = uefi_call_wrapper(BS->OpenProtocol, 6, gImageHandle, &gEfiLoadedImageProtocolGuid, 
                                          (void**)&LoadedImage, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(Status)) {
		Print(L"Unable to access boot image interface\n");
	}

    /*
     * Disconnect Blocking Drivers
     * to prevent weird issues.
    */
    DisconnectBlockingDrivers();

    /*
     * Get Boot Partition Path to
     * skip later.
    */
    EFI_DEVICE_PATH* BootPartitionPath = DevicePathFromHandle(LoadedImage->DeviceHandle);
	EFI_DEVICE_PATH* BootDiskPath = GetParentDevice(BootPartitionPath);

    /*
     * Get all the Device Handles
    */
    uint64_t HandleCount;
    EFI_HANDLE* Handles;
    Status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to list disks\n");
    }

    for (uint64_t i = 0; i < HandleCount; i++) {
        EFI_DEVICE_PATH* DevicePath = DevicePathFromHandle(Handles[i]);
        if (CompareDevicePaths(DevicePath, BootPartitionPath) == 0) {
            continue;
        }

        /*
         * Skip devices we didn't
         * boot from.
        */
        EFI_DEVICE_PATH* ParentDevicePath = GetParentDevice(DevicePath);
        if (CompareDevicePaths(BootDiskPath, ParentDevicePath) != 0) {
            continue;
        }

        if (Handles[i] == NULL) {
			Print(L"Handle is NULL\n");
            continue;
        }

        /*
         * Get Block IO for the current
         * handle
        */
        EFI_BLOCK_IO_PROTOCOL* BlockIo;
        EFI_GUID bioGuid = BLOCK_IO_PROTOCOL;
        Status = uefi_call_wrapper(BS->OpenProtocol, 6, Handles[i], &bioGuid, (void**)&BlockIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status) || BlockIo == NULL || BlockIo->Media->BlockSize == 0) {
            Print(L"HandleProtocol failed on Handle[%d]: %r\n", i, Status);
            continue;
        }

        /*
         * Skip stuff we don't need.
        */
        if (!BlockIo->Media->MediaPresent || BlockIo->Media->LogicalPartition == false) {
            continue;
        }

        /*
         * Now we need to allocate a buffer
         * to check what filesystem it is.
        */
        char* Buffer = (char*)AllocatePool(BlockIo->Media->BlockSize);
        if (Buffer == NULL) {
            continue;
        }

        Status = uefi_call_wrapper(BlockIo->ReadBlocks, 5, BlockIo, BlockIo->Media->MediaId, ((1024 / BlockIo->Media->BlockSize) > 0 ? (1024 / BlockIo->Media->BlockSize) : 0), BlockIo->Media->BlockSize, Buffer);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to read first block: %r\n", Status);
            FreePool(Buffer);
            continue;
        }

        /*
         * We can check what filesystem
         * the partition is using by using
         * the FS Magic Table below.
        */
        const char FsMagic[3][8] = {
            { 'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '},
            { 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '},
            { 0x53, 0xEF, 0, 0, 0, 0, 0, 0 }
        };

        enum FSType FsIndex;
        if (CompareMem(Buffer + 0x03, FsMagic[0], 8) == 0) {
            FsIndex = NTFS;
        } else if (CompareMem(Buffer + 0x52, FsMagic[1], 8) == 0) {
            FsIndex = EXFAT;
        } else if (CompareMem(Buffer + 56, FsMagic[2], 2) == 0) {
            FsIndex = EXT;
        } else {
            Print(L"Error: Unknown filesystem\n");
            FreePool(Buffer);
            continue;
        }

        FreePool(Buffer);

        /*
         * We need to unload the
         * uefi' drivers before
         * we load ours.
        */
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume;
        Status = uefi_call_wrapper(BS->OpenProtocol, 6, Handles[i], &gEfiSimpleFileSystemProtocolGuid, (void**)&Volume, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
        if (Status != EFI_SUCCESS && Status != EFI_UNSUPPORTED) {
            const wchar_t* FsName[] = { L"NTFS", L"exFAT", L"ext4" };
            Print(L"Could not check for %s service", FsName[FsIndex]);
        }

        if (Status == EFI_SUCCESS) {
            if (UnloadDriver(Handles[i]) == EFI_SUCCESS) {
                Status = EFI_UNSUPPORTED;
			} else {
                const wchar_t* FsName[] = { L"NTFS", L"exFAT", L"ext4" };
				Print(L"Could not unload %s driver\n", FsName[FsIndex]);
			}
        }

        /*
         * If the filesystem is unsupported
         * we can load our drivers.
        */
        if (Status == EFI_UNSUPPORTED) {
            const wchar_t* DriverName[] = { L"ntfs", L"exfat", L"ext2" };
            wchar_t DriverPath[64];

            SPrint(DriverPath, ARRAY_SIZE(DriverPath), L"\\EFI\\AstralBoot\\%s_%s.efi", DriverName[FsIndex], L"x64");
            DevicePath = FileDevicePath(LoadedImage->DeviceHandle, DriverPath);
            if (DevicePath == NULL) {
                Status = EFI_DEVICE_ERROR;
                Print(L"Unable to set path for '%s'\n", DriverPath);
            }

            Status = uefi_call_wrapper(BS->LoadImage, 6, false, gImageHandle, DevicePath, NULL, 0, &ImageHandle);
            SafeFree(DevicePath);
            if (EFI_ERROR(Status)) {
                if ((Status == EFI_ACCESS_DENIED) && (SecureBootStatus >= 1)) {
                    Status = EFI_SECURITY_VIOLATION;
                }
                Print(L"Unable to load driver '%s'\n", DriverPath);
            }

            Status = uefi_call_wrapper(BS->OpenProtocol, 6, ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
            if (EFI_ERROR(Status)) {
                Print(L"Unable to access driver interface\n");
            }

            if (LoadedImage->ImageCodeType != EfiBootServicesCode) {
                Status = EFI_LOAD_ERROR;
                Print(L"'%s' is not a Boot System Driver\n", DriverPath);
            }

            Status = uefi_call_wrapper(BS->StartImage, 3, ImageHandle, NULL, NULL);
            if (EFI_ERROR(Status)) {
                Print(L"Unable to start driver\n");
            }

            Status = uefi_call_wrapper(BS->ConnectController, 4, Handles[i], &ImageHandle, NULL, TRUE);
            if (EFI_ERROR(Status)) {
                const wchar_t* FsName[] = { L"NTFS", L"exFAT", L"ext4" };
                Print(L"Could not start %s partition service\n", FsName[FsIndex]);
            }
        }

        /*
         * Why wchar_t's ??
        */
        wchar_t LoaderPath[64];
        SPrint(LoaderPath, ARRAY_SIZE(LoaderPath), L"\\AstralOS\\System64\\kernel.elf", L"x64");

        while (true) {
            Status = uefi_call_wrapper(BS->OpenProtocol, 6, Handles[i], &gEfiSimpleFileSystemProtocolGuid, (void**)&Volume, gImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
            if (!EFI_ERROR(Status)) {
                break;
            }

            Print(L"Could not open partition");
            Print(L"Waiting %d seconds before retrying...\n", 3);
            uefi_call_wrapper(BS->Stall, 1, 3 * 1000000);
        }

        EFI_FILE_HANDLE Root = NULL;
        Status = uefi_call_wrapper(Volume->OpenVolume, 2, Volume, &Root);
        if ((EFI_ERROR(Status)) || (Root == NULL)) {
            Print(L"Could not open Root directory\n");
        }

        uint64_t size = FILE_INFO_SIZE;
        EFI_FILE_SYSTEM_VOLUME_LABEL_INFO* VolumeInfo = (EFI_FILE_SYSTEM_VOLUME_LABEL_INFO*)AllocateZeroPool(size);
        if (VolumeInfo != NULL) {
            Status = uefi_call_wrapper(Root->GetInfo, 4, Root, &gEfiFileSystemVolumeLabelInfoIdGuid, &size, VolumeInfo);
            /*
             * Some UEFI firmwares return EFI_BUFFER_TOO_SMALL, even with
             * a large enough buffer, unless the exact size is requested.
            */
            if ((Status == EFI_BUFFER_TOO_SMALL) && (size <= FILE_INFO_SIZE)) {
                Status = uefi_call_wrapper(Root->GetInfo, 4, Root, &gEfiFileSystemVolumeLabelInfoIdGuid, &size, VolumeInfo);
            }

            if (Status == EFI_SUCCESS) {
                Print(L"Volume label is '%s'\n", VolumeInfo->VolumeLabel);
            } else {
                Print(L"Could not read volume label: [%d] %r\n", (Status & 0x7FFFFFFF), Status);
            }

            FreePool(VolumeInfo);
        }

        DevicePath = FileDevicePath(Handles[i], LoaderPath);
        if (DevicePath == NULL) {
            Status = EFI_DEVICE_ERROR;
            Print(L"Could not create path\n");
        }

        EFI_FILE_HANDLE FileHandle;
        Status = uefi_call_wrapper(Root->Open, 5, Root, &FileHandle, LoaderPath, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) {
            Print(L"Could not open file '%s': %r\n", LoaderPath, Status);
            continue;
        }

        /*
         * Now we can load the kernel
        */
        uint64_t FileSize = 0;
        Status = uefi_call_wrapper(FileHandle->GetInfo, 4, FileHandle, &gEfiFileInfoGuid, &FileSize, NULL);
        if (Status == EFI_BUFFER_TOO_SMALL) {
            void* FileBuffer = AllocatePool(FileSize);
            if (FileBuffer == NULL) {
                Print(L"Failed to allocate memory for file buffer\n");
                continue;
            }

            Status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &FileSize, FileBuffer);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to read file: %r\n", Status);
                FreePool(FileBuffer);
                continue;
            }

            Elf64_Ehdr* hdr = GetELFHeader(FileBuffer);
            if (ValidateEhdr(hdr)) {
                //ks->basicConsole.Println("ELF Driver is Valid!");
            } else {
                Print(L"ELF is Invalid!\n");
                FreePool(FileBuffer);
            }

            uint64_t entry = LoadElf(hdr);
            if (!entry) {
                Print(L"ELF Loading has Failed!\n");
            }

            EFI_STATUS status = uefi_call_wrapper(FileHandle->Close, 1, FileHandle);

            if (BlockIo) {
                Status = uefi_call_wrapper(BS->CloseProtocol, 4, Handles[i], &bioGuid, gImageHandle, NULL);
            }

            /*
             * Load The Init RAM FS
             * 
             * Here, we load it as a .efi file because
             * of VBox' weird quirks.
            */
            EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* ESPVolume = NULL;
            Status = uefi_call_wrapper(BS->HandleProtocol, 3, LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&ESPVolume);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to get ESP volume protocol: %r\n", Status);
                return Status;
            }

            EFI_FILE_HANDLE ESPRoot = NULL;
            Status = uefi_call_wrapper(ESPVolume->OpenVolume, 2, ESPVolume, &ESPRoot);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to open ESP root: %r\n", Status);
                return Status;
            }

            EFI_FILE_HANDLE InitrdHandle;
            CHAR16* InitrdPath = L"\\EFI\\AstralBoot\\initramfs.efi";
            Status = uefi_call_wrapper(ESPRoot->Open, 5, ESPRoot, &InitrdHandle, InitrdPath, EFI_FILE_MODE_READ, 0);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to open initramfs on ESP: %r\n", Status);
                return Status;
            }

            uint64_t InitrdInfoSize = 0;
            Status = uefi_call_wrapper(InitrdHandle->GetInfo, 4, InitrdHandle, &gEfiFileInfoGuid, &InitrdInfoSize, NULL);
            if (Status != EFI_BUFFER_TOO_SMALL) {
                Print(L"Failed to get initramfs info size: %r\n", Status);
                return Status;
            }

            EFI_FILE_INFO* InitrdInfo = AllocatePool(InitrdInfoSize);
            if (!InitrdInfo) {
                Print(L"Failed to allocate pool for initrd info\n");
                return EFI_OUT_OF_RESOURCES;
            }

            Status = uefi_call_wrapper(InitrdHandle->GetInfo, 4, InitrdHandle, &gEfiFileInfoGuid, &InitrdInfoSize, InitrdInfo);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to get initrd info: %r\n", Status);
                FreePool(InitrdInfo);
                return Status;
            }

            uint64_t InitrdSize = InitrdInfo->FileSize;
            FreePool(InitrdInfo);

            void* InitrdBuffer = AllocatePool(InitrdSize);
            if (!InitrdBuffer) {
                Print(L"Failed to allocate buffer for initramfs\n");
                return EFI_OUT_OF_RESOURCES;
            }

            Status = uefi_call_wrapper(InitrdHandle->Read, 3, InitrdHandle, &InitrdSize, InitrdBuffer);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to read initramfs: %r\n", Status);
                FreePool(InitrdBuffer);
                return Status;
            }

            Print(L"initramfs loaded at 0x%lx (%llu bytes)\n", (uint64_t)InitrdBuffer, InitrdSize);

            /*
             * Now we can get all the stuff
             * we need to initialize our
             * kernel.
            */
            InitializeGOP();

            EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput;
            Status = uefi_call_wrapper(gBS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL, (void**)&GraphicsOutput);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to locate Graphics Output Protocol\n");
                return Status;
            }

            Print(L"ss\n");

            if (Handles) {
                FreePool(Handles);
                Print(L"Handles freed\n");
            }

            if (Buffer) {
                FreePool(Buffer);
                Print(L"Buffer freed\n");
            }
/*
            BootInfo bi;
			bi.pFramebuffer = framebuffer;
            bi.rsdp = rsdsp;
            bi.initrdBase = InitrdBuffer;
            bi.initrdSize = InitrdSize;*/

            
            Print(L"ff\n");

            for (uint64_t offset = 0; offset < InitrdSize; offset += 0x1000) {
                MapMemory((void*)((uint64_t)InitrdBuffer + HIGHER_VIRT_ADDR + offset), (void*)((uint64_t)InitrdBuffer + offset));
            }

            for (uint64_t offset = 0; offset < FileBuffer; offset += 0x1000) {
                MapMemory((void*)(KERNEL_VIRT_ADDR + offset), (void*)((uint64_t)FileBuffer + offset));
            }

            for (uint64_t offset = 0; offset < framebuffer.BufferSize; offset += 0x1000) {
                MapMemory((void*)((uint64_t)framebuffer.BaseAddress + HIGHER_VIRT_ADDR + offset), (void*)((uint64_t)framebuffer.BaseAddress + offset));
            }

            

            //asm volatile ("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
        }
    }

    return EFI_SUCCESS;
}