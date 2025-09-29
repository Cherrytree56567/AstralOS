#include "main.h"
#include "elf.h"

EFI_GUID Acpi20TableGuid = { 0x8868e871, 0xe4f1, 0x11d3, {0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81} };

EFI_GUID Acpi10TableGuid = {
    0xeb9d2d30, 0x2d88, 0x11d3,
    { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d }
};

void DumpMemory(EFI_SYSTEM_TABLE* SystemTable, void* addr, UINTN length) {
    if (addr == NULL) {
        Print(L"NULL pointer, cannot dump.\r\n");
        return;
    }

    CHAR8* sig = (CHAR8*)addr;
    if (!(sig[0] == 'R' && sig[1] == 'S' && sig[2] == 'D' && sig[3] == ' ' &&
          sig[4] == 'P' && sig[5] == 'T' && sig[6] == 'R' && sig[7] == ' ')) {
        Print(L"Invalid RSDP signature at %p\r\n", addr);
        return;
    }

    UINT8* bytes = (UINT8*)addr;
    for (UINTN i = 0; i < length; i++) {
        if (i % 16 == 0) {
            SystemTable->ConOut->OutputString(SystemTable->ConOut, L"\r\n");
            Print(L"%08lx: ", (UINT64)(uintptr_t)(bytes + i));
        }
        Print(L"%02x ", bytes[i]);
    }
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"\r\n");
}

void* FindRSDP(EFI_SYSTEM_TABLE* SystemTable) {
    void* rsdp = NULL;

    if (LibGetSystemConfigurationTable(&Acpi20TableGuid, &rsdp) == EFI_SUCCESS && rsdp != NULL) {
        CHAR8* sig = (CHAR8*)rsdp;
        if (CompareMem(sig, "RSD PTR ", 8) == 0) {
            Print(L"Found ACPI 2.0 RSDP at: %p\r\n", rsdp);
        } else {
            Print(L"Invalid ACPI 2.0 signature\r\n");
        }
    } else if (LibGetSystemConfigurationTable(&Acpi10TableGuid, &rsdp) == EFI_SUCCESS && rsdp != NULL) {
        CHAR8* sig = (CHAR8*)rsdp;
        if (CompareMem(sig, "RSD PTR ", 8) == 0) {
            Print(L"Found ACPI 1.0 RSDP at: %p\r\n", rsdp);
        } else {
            Print(L"Invalid ACPI 1.0 signature\r\n");
        }
    } else {
        Print(L"RSDP not found in UEFI system table.\r\n");
    }
    return rsdp;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    /*
	* FSMAGIC, used to check if partition is NTFS, EXT4 or exFAT
    */
    CONST CHAR8 FsMagic[3][8] = {
        { 'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '},
        { 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '},
        { 0x53, 0xEF, 0, 0, 0, 0, 0, 0 } // ext4 magic number in Little Endian
    };

    CONST CHAR16* FsName[] = { L"NTFS", L"exFAT", L"ext4" };
    CONST CHAR16* DriverName[] = { L"ntfs", L"exfat", L"ext2" };
	EFI_STATUS Status;
	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	EFI_DEVICE_PATH* BootDiskPath = NULL;
	EFI_DEVICE_PATH* BootPartitionPath = NULL;
	CHAR16* DevicePathString;
    EFI_HANDLE* Handles = NULL;
    UINTN HandleCount = 0;
    UINTN Index = 0;
    EFI_INPUT_KEY Key;
    EFI_DEVICE_PATH* DevicePath = NULL;
    EFI_DEVICE_PATH* ParentDevicePath = NULL;
    BOOLEAN SameDevice;
    CHAR8* Buffer;
    EFI_BLOCK_IO_PROTOCOL* BlockIo;
    int FsIndex = -1;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume;
    CONST EFI_GUID bioGuid = BLOCK_IO_PROTOCOL;
    INTN SecureBootStatus;
    CHAR16 DriverPath[64];
    CHAR16 LoaderPath[64];
    EFI_HANDLE* DriverHandleList[2];
    UINTN Try;
    EFI_FILE_HANDLE Root;
    UINTN Size;
    EFI_FILE_SYSTEM_VOLUME_LABEL* VolumeInfo;
    EFI_FILE_HANDLE FileHandle;
    CHAR8* FileBuffer;
    UINTN FileSize = 0;
    EFI_HANDLE CorrectHandle = NULL;

	InitializeLib(ImageHandle, SystemTable);
	gImageHandle = ImageHandle;

    void* rsdsp = FindRSDP(SystemTable);
    if (rsdsp == NULL) {
        Print(L"RSDP not found\n");
        return EFI_NOT_FOUND;
    }

    SecureBootStatus = GetSecureBootStatus();

	/*
	* Get Current Boot Image to get the device path
	*/
	Status = uefi_call_wrapper(BS->OpenProtocol, 6, gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(Status)) {
		Print(L"Unable to access boot image interface\n");
	}
	
    /*
	* Disconnect potentially blocking drivers
    */
	DisconnectBlockingDrivers();

	/*
	* Get the boot disk and partition paths
    */
	BootPartitionPath = DevicePathFromHandle(LoadedImage->DeviceHandle);
	BootDiskPath = GetParentDevice(BootPartitionPath);

    /*
	* Print the boot disk and partition paths
    */
	DevicePathString = ConvertDevicePathToText(BootDiskPath);
	SafeFree(DevicePathString);

    /*
	* List all the disk handles
    */
    Status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to list disks\n");
    }

    for (Index = 0; Index < HandleCount; Index++) {
        DevicePath = DevicePathFromHandle(Handles[Index]);
        if (CompareDevicePaths(DevicePath, BootPartitionPath) == 0)
            continue;
        ParentDevicePath = GetParentDevice(DevicePath);
        SameDevice = (CompareDevicePaths(BootDiskPath, ParentDevicePath) == 0);
        if (!SameDevice)
            continue;

        if (Handles[Index] == NULL)
			Print(L"Handle is NULL\n");


        Status = uefi_call_wrapper(BS->OpenProtocol, 6, Handles[Index], &bioGuid, (VOID**)&BlockIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status) || BlockIo == NULL || BlockIo->Media->BlockSize == 0) {
            Print(L"HandleProtocol failed on Handle[%d]: %r\n", Index, Status);
            continue;
        }

        Buffer = (CHAR8*)AllocatePool(BlockIo->Media->BlockSize);
        if (Buffer == NULL)
            continue;

        if (!BlockIo->Media->MediaPresent || BlockIo->Media->LogicalPartition == FALSE) {
            continue;
        }

        Status = uefi_call_wrapper(BlockIo->ReadBlocks, 5, BlockIo, BlockIo->Media->MediaId, ((1024 / BlockIo->Media->BlockSize) > 0 ? (1024 / BlockIo->Media->BlockSize) : 0), BlockIo->Media->BlockSize, Buffer);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to read first block: %r\n", Status);
            FreePool(Buffer);
            continue;
        }

        if (CompareMem(Buffer + 0x03, FsMagic[0], 8) == 0) {
            FsIndex = 0;
        } else if (CompareMem(Buffer + 0x52, FsMagic[1], 8) == 0) {
            FsIndex = 1;
        } else if (CompareMem(Buffer + 56, FsMagic[2], 2) == 0) {
            FsIndex = 2;
        } else {
            Print(L"Error: Unknown filesystem\n");
            FreePool(Buffer);
            continue;
        }

        FreePool(Buffer);

        Status = uefi_call_wrapper(BS->OpenProtocol, 6, Handles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Volume, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);

        if (Status != EFI_SUCCESS && Status != EFI_UNSUPPORTED) {
            Print(L"Could not check for %s service", FsName[FsIndex]);
        }

        if (Status == EFI_SUCCESS) {
            if (UnloadDriver(Handles[Index]) == EFI_SUCCESS) {
                Status = EFI_UNSUPPORTED;
			} else {
				Print(L"Could not unload %s driver\n", FsName[FsIndex]);
			}
        }

        if (Status == EFI_UNSUPPORTED) {

            UnicodeSPrint(DriverPath, ARRAY_SIZE(DriverPath), L"\\EFI\\AstralBoot\\%s_%s.efi", DriverName[FsIndex], Arch);
            DevicePath = FileDevicePath(LoadedImage->DeviceHandle, DriverPath);
            if (DevicePath == NULL) {
                Status = EFI_DEVICE_ERROR;
                Print(L"Unable to set path for '%s'\n", DriverPath);
            }

            Status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, gImageHandle, DevicePath, NULL, 0, &ImageHandle);
            SafeFree(DevicePath);
            if (EFI_ERROR(Status)) {
                if ((Status == EFI_ACCESS_DENIED) && (SecureBootStatus >= 1))
                    Status = EFI_SECURITY_VIOLATION;
                Print(L"Unable to load driver '%s'\n", DriverPath);
            }

            Status = uefi_call_wrapper(BS->OpenProtocol, 6, ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
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

            DriverHandleList[0] = ImageHandle;
            DriverHandleList[1] = NULL;
            Status = uefi_call_wrapper(BS->ConnectController, 4, Handles[Index], DriverHandleList, NULL, TRUE);
            if (EFI_ERROR(Status)) {
                Print(L"Could not start %s partition service\n", FsName[FsIndex]);
            }
        }

        UnicodeSPrint(LoaderPath, ARRAY_SIZE(LoaderPath), L"\\AstralOS\\System64\\kernel.elf", Arch);
        
        for (Try = 0; ; Try++) {
            Status = uefi_call_wrapper(BS->OpenProtocol, 6, Handles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Volume, gImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
            if (!EFI_ERROR(Status))
                break;
            Print(L"Could not open partition");
            Print(L"Waiting %d seconds before retrying...\n", 3);
            uefi_call_wrapper(BS->Stall, 1, 3 * 1000000);
        }

        Root = NULL;
        Status = uefi_call_wrapper(Volume->OpenVolume, 2, Volume, &Root);
        if ((EFI_ERROR(Status)) || (Root == NULL)) {
            Print(L"Could not open Root directory\n");
        }

        Size = FILE_INFO_SIZE;
        VolumeInfo = (EFI_FILE_SYSTEM_VOLUME_LABEL*)AllocateZeroPool(Size);
        if (VolumeInfo != NULL) {
            Status = uefi_call_wrapper(Root->GetInfo, 4, Root, &gEfiFileSystemVolumeLabelInfoIdGuid, &Size, VolumeInfo);
            // Some UEFI firmwares return EFI_BUFFER_TOO_SMALL, even with
            // a large enough buffer, unless the exact size is requested.
            if ((Status == EFI_BUFFER_TOO_SMALL) && (Size <= FILE_INFO_SIZE))
                Status = uefi_call_wrapper(Root->GetInfo, 4, Root, &gEfiFileSystemVolumeLabelInfoIdGuid, &Size, VolumeInfo);
            if (Status == EFI_SUCCESS)
                Print(L"Volume label is '%s'\n", VolumeInfo->VolumeLabel);
            else
                Print(L"Could not read volume label: [%d] %r\n", (Status & 0x7FFFFFFF), Status);
            FreePool(VolumeInfo);
        }

        DevicePath = FileDevicePath(Handles[Index], LoaderPath);
        if (DevicePath == NULL) {
            Status = EFI_DEVICE_ERROR;
            Print(L"Could not create path\n");
        }

        Status = uefi_call_wrapper(Root->Open, 5, Root, &FileHandle, LoaderPath, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) {
            Print(L"Could not open file '%s': %r\n", LoaderPath, Status);
            continue;
        }

        Status = uefi_call_wrapper(FileHandle->GetInfo, 4, FileHandle, &gEfiFileInfoGuid, &FileSize, NULL);
        if (Status == EFI_BUFFER_TOO_SMALL) {
            FileBuffer = AllocatePool(FileSize);
            if (FileBuffer == NULL) {
                Print(L"Failed to allocate memory for file buffer\n");
                continue;
            }

            Elf64_Ehdr ehdr;
            UINTN size = sizeof(ehdr);
			Status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &size, &ehdr);

            if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
                ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
                Print(L"Invalid ELF file\n");
                return EFI_LOAD_ERROR;
            }
            
            uint64_t phys_load_base = 0x1000;
            uint64_t virt_load_base = 0xFFFFFFFF80000000;

            for (int i = 0; i < ehdr.e_phnum; i++) {
                // Seek to program header
                Elf64_Phdr phdr;
                UINT64 offset = ehdr.e_phoff + i * sizeof(phdr);
                Status = uefi_call_wrapper(FileHandle->SetPosition, 2, FileHandle, offset);
                size = sizeof(phdr);
                Status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &size, &phdr);

                // Only load segments with type PT_LOAD
                if (phdr.p_type == PT_LOAD) {
                    // Allocate memory
                    UINT64 seg_offset = phdr.p_vaddr - virt_load_base;
                    EFI_PHYSICAL_ADDRESS addr = phdr.p_paddr;
                    Print(L"Offset: 0x%lx", addr);
                    Print(L"Addr: 0x%lx", phdr.p_paddr);
                    EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(phdr.p_memsz), &addr);
                    if (EFI_ERROR(status)) {
                        Print(L"Failed to allocate pages: %r\n", status);
                        return status;
                    }

                    // Load the segment data
                    Status = uefi_call_wrapper(FileHandle->SetPosition, 2, FileHandle, phdr.p_offset);
                    size = phdr.p_filesz;
                    Status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &size, (void*)phdr.p_paddr);

                    // Zero out remaining memory (bss)
                    if (phdr.p_memsz > phdr.p_filesz) {
                        ZeroMem((void*)(phdr.p_paddr + phdr.p_filesz), phdr.p_memsz - phdr.p_filesz);
                    }
                }
            }

            EFI_STATUS status = uefi_call_wrapper(FileHandle->Close, 1, FileHandle);

            if (BlockIo) {
                Status = uefi_call_wrapper(BS->CloseProtocol, 6, Handles[Index], &bioGuid, BlockIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
            }

            EFI_FILE_HANDLE InitrdHandle;
            EFI_FILE_INFO* InitrdInfo;
            UINTN InitrdInfoSize = 0;
            VOID* InitrdBuffer = NULL;
            UINTN InitrdSize = 0;

            EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* ESPVolume = NULL;
            EFI_FILE_HANDLE ESPRoot = NULL;

            Status = uefi_call_wrapper(BS->HandleProtocol,
                3,
                LoadedImage->DeviceHandle,
                &gEfiSimpleFileSystemProtocolGuid,
                (VOID**)&ESPVolume);

            if (EFI_ERROR(Status)) {
                Print(L"Failed to get ESP volume protocol: %r\n", Status);
                return Status;
            }

            Status = uefi_call_wrapper(ESPVolume->OpenVolume, 2, ESPVolume, &ESPRoot);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to open ESP root: %r\n", Status);
                return Status;
            }

            // Try to open initramfs on the ESP
            CHAR16* InitrdPath = L"\\EFI\\AstralBoot\\initramfs.efi";
            Status = uefi_call_wrapper(ESPRoot->Open, 5, ESPRoot, &InitrdHandle, InitrdPath, EFI_FILE_MODE_READ, 0);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to open initramfs on ESP: %r\n", Status);
                return Status;
            }

            // Get file size
            Status = uefi_call_wrapper(InitrdHandle->GetInfo, 4, InitrdHandle, &gEfiFileInfoGuid, &InitrdInfoSize, NULL);
            if (Status != EFI_BUFFER_TOO_SMALL) {
                Print(L"Failed to get initramfs info size: %r\n", Status);
                return Status;
            }

            InitrdInfo = AllocatePool(InitrdInfoSize);
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

            InitrdSize = InitrdInfo->FileSize;
            FreePool(InitrdInfo);

            // Allocate buffer and read it
            InitrdBuffer = AllocatePool(InitrdSize);
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

            Print(L"initramfs loaded at %p (%llu bytes)\n", InitrdBuffer, InitrdSize);

			InitializeGOP();

            EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput;

            // Locate the Graphics Output Protocol
            Status = uefi_call_wrapper(gBS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&GraphicsOutput);
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

            BootInfo bi;
			bi.pFramebuffer = framebuffer;
            bi.rsdp = rsdsp;
            bi.initrdBase = InitrdBuffer;
            bi.initrdSize = InitrdSize;

            Print(L"nn\n");

            int (*kernel_main)(BootInfo*) = (int (*)(BootInfo*))ehdr.e_entry;

            // Clear the screen by filling it with black
            UINTN Width = GraphicsOutput->Mode->Info->HorizontalResolution;
            UINTN Height = GraphicsOutput->Mode->Info->VerticalResolution;
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL BlackPixel = { 0, 0, 0, 0 }; // RGBA format

            /*
            * Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-5-Efi-Memory-Map/gnu-efi/bootloader/main.c
            */
            EFI_MEMORY_DESCRIPTOR* Map = NULL;
            UINTN MapSize, MapKey;
            UINTN DescriptorSize;
            UINT32 DescriptorVersion;

            SafeFree(ParentDevicePath);

            // First call to get size
            MapSize = 0;
            Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to GGet memory map: %r\n", Status);
            }

            MapSize += DescriptorSize;

            // Allocate map
            Status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, MapSize, (void**)&Map);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to allocate memory map: %r\n", Status);
                return Status;
            }

            // Actual call
            Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
            if (EFI_ERROR(Status)) {
                Print(L"Failed to get memory map: %r\n", Status);
                return Status;
            }

            status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, MapKey);
            if (EFI_ERROR(status)) {
                Print(L"Failed to exit boot services: %r\n", status);
                return status; // Handle error appropriately
            }

            bi.mMap = Map;
			bi.mMapSize = MapSize;
			bi.mMapDescSize = DescriptorSize;

            int return_value = kernel_main(&bi);

            return EFI_SUCCESS;
        }
    }
}