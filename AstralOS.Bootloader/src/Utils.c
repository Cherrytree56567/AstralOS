#include "Utils.h"

/*
 * We can find the RSDP by checking
 * the System Config Table.
*/
EFI_GUID Acpi20TableGuid = { 0x8868e871, 0xe4f1, 0x11d3, {0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81} };

EFI_GUID Acpi10TableGuid = {
    0xeb9d2d30, 0x2d88, 0x11d3,
    { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d }
};
void* FindRSDP(EFI_SYSTEM_TABLE* SystemTable) {
    void* rsdp = NULL;

    if (LibGetSystemConfigurationTable(&Acpi20TableGuid, &rsdp) == EFI_SUCCESS && rsdp != NULL) {
        char* sig = (char*)rsdp;

        if (CompareMem(sig, "RSD PTR ", 8) != 0) {
            Print(L"Invalid ACPI 2.0 signature\r\n");
        }
    } else if (LibGetSystemConfigurationTable(&Acpi10TableGuid, &rsdp) == EFI_SUCCESS && rsdp != NULL) {
        char* sig = (char*)rsdp;

        if (CompareMem(sig, "RSD PTR ", 8) != 0) {
            Print(L"Invalid ACPI 1.0 signature\r\n");
        }
    } else {
        Print(L"RSDP not found in UEFI system table.\r\n");
    }
    return rsdp;
}

/*
* Get the Secure Boot Status
* From pbatard/uefi-ntfs
* https://github.com/pbatard/uefi-ntfs
*/
int64_t GetSecureBootStatus() {
    uint8_t SecureBoot = 0;
    int64_t SecureBootStatus = 0;

    uint64_t Size = sizeof(SecureBoot);
    EFI_STATUS Status = uefi_call_wrapper(RT->GetVariable, 5, L"SecureBoot", &gEfiGlobalVariableGuid, NULL, &Size, &SecureBoot);

    if (Status == EFI_SUCCESS) {
        uint8_t SetupMode;
        SecureBootStatus = (INTN)SecureBoot;

        Size = sizeof(SetupMode);
        Status = uefi_call_wrapper(RT->GetVariable, 5, L"SetupMode", &gEfiGlobalVariableGuid, NULL, &Size, &SetupMode);

        if (Status == EFI_SUCCESS && SetupMode != 0) {
            SecureBootStatus = -1;
        }
    }

    return SecureBootStatus;
}

/*
 * Some UEFI firmwares (like HPQ EFI from HP notebooks) have DiskIo protocols
 * opened BY_DRIVER (by Partition driver in HP's case) even when no file system
 * is produced from this DiskIo. This then blocks our FS driver from connecting
 * and producing file systems.
 * To fix it we disconnect drivers that connected to DiskIo BY_DRIVER if this
 * is a partition volume and if those drivers did not produce file system.
 *
 * This code was originally derived from similar BSD-3-Clause licensed one
 * (a.k.a. Modified BSD License, which can be used in GPLv2+ works), found at:
 * https://sourceforge.net/p/cloverefiboot/code/3294/tree/rEFIt_UEFI/refit/main.c#l1271
 */
void DisconnectBlockingDrivers() {
	uint64_t HandleCount = 0;
	EFI_HANDLE* Handles = NULL;
	
	EFI_STATUS Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &HandleCount, &Handles);
	if (EFI_ERROR(Status) || (HandleCount == 0)) {
		return;
    }

	for (uint64_t Index = 0; Index < HandleCount; Index++) {
        EFI_BLOCK_IO_PROTOCOL* BlockIo;
		Status = gBS->OpenProtocol(Handles[Index], &gEfiBlockIoProtocolGuid, (VOID**)&BlockIo,
			gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

		if (EFI_ERROR(Status)) {
			continue;
        }

		if ((BlockIo->Media == NULL) || (!BlockIo->Media->LogicalPartition)) {
			continue;
        }

        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume;
		Status = gBS->OpenProtocol(Handles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Volume,
			gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		if (Status == EFI_SUCCESS) {
			continue;
        }

		uint16_t* DevicePathString = DevicePathToString(DevicePathFromHandle(Handles[Index]));

        EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfo;
        uint64_t OpenInfoCount;
		Status = gBS->OpenProtocolInformation(Handles[Index], &gEfiDiskIoProtocolGuid, &OpenInfo, &OpenInfoCount);
		if (EFI_ERROR(Status)) {
			PrintWarning(L"  Could not get DiskIo protocol for %s: %r", DevicePathString, Status);
			FreePool(DevicePathString);
			continue;
		}

		for (uint64_t OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
			if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) == EFI_OPEN_PROTOCOL_BY_DRIVER) {
				Status = gBS->DisconnectController(Handles[Index], OpenInfo[OpenInfoIndex].AgentHandle, NULL);
				if (EFI_ERROR(Status)) {
					Print(L"  Could not disconnect '%s' on %s",
						GetDriverName(OpenInfo[OpenInfoIndex].AgentHandle), DevicePathString);
				} else {
					Print(L"  Disconnected '%s' on %s ",
						GetDriverName(OpenInfo[OpenInfoIndex].AgentHandle), DevicePathString);
				}
			}
		}
		FreePool(DevicePathString);
		FreePool(OpenInfo);
	}
	FreePool(Handles);
}

/*
* Get the Graphics Output Protocol
* 
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-3-Graphics-Output-Protocol/gnu-efi/bootloader/main.c
*/
Framebuffer* InitializeGOP() {
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if (EFI_ERROR(status)) {
        Print(L"Unable to locate GOP\n\r");
        return NULL;
    } else {
        Print(L"GOP located\n\r");
    }

    framebuffer.BaseAddress = (void*)gop->Mode->FrameBufferBase;
    framebuffer.BufferSize = gop->Mode->FrameBufferSize;
    framebuffer.Width = gop->Mode->Info->HorizontalResolution;
    framebuffer.Height = gop->Mode->Info->VerticalResolution;
    framebuffer.PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;

    return &framebuffer;
}

uint16_t* GetDriverName(const EFI_HANDLE DriverHandle) {
    uint16_t* DriverName;
    EFI_COMPONENT_NAME_PROTOCOL* ComponentName;
    EFI_COMPONENT_NAME2_PROTOCOL* ComponentName2;

    // Try EFI_COMPONENT_NAME2 protocol first
    if ((uefi_call_wrapper(BS->OpenProtocol, 6, DriverHandle, &gEfiComponentName2ProtocolGuid, (VOID**)&ComponentName2, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL) == EFI_SUCCESS) &&
        (ComponentName2->GetDriverName(ComponentName2, ComponentName2->SupportedLanguages, &DriverName) == EFI_SUCCESS))
        return DriverName;

    // Fallback to EFI_COMPONENT_NAME if that didn't work
    if ((uefi_call_wrapper(BS->OpenProtocol, 6, DriverHandle, &gEfiComponentNameProtocolGuid, (VOID**)&ComponentName, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL) == EFI_SUCCESS) &&
        (ComponentName->GetDriverName(ComponentName, ComponentName->SupportedLanguages, &DriverName) == EFI_SUCCESS))
        return DriverName;

    return L"(unknown driver)";
}

EFI_DEVICE_PATH* GetLastDevicePath(const EFI_DEVICE_PATH_PROTOCOL* dp) {
    EFI_DEVICE_PATH* next, * p;

    if (IsDevicePathEnd(dp)) {
        return NULL;
    }

    for (p = (EFI_DEVICE_PATH*)dp, next = NextDevicePathNode(p); !IsDevicePathEnd(next); p = next, next = NextDevicePathNode(next)) {

    }

    return p;
}

EFI_DEVICE_PATH* GetParentDevice(const EFI_DEVICE_PATH* DevicePath) {
    EFI_DEVICE_PATH* dp, * ldp;

    dp = DuplicateDevicePath((EFI_DEVICE_PATH*)DevicePath);
    if (dp == NULL) {
        return NULL;
    }

    ldp = GetLastDevicePath(dp);
    if (ldp == NULL) {
        return NULL;
    }

    ldp->Type = END_DEVICE_PATH_TYPE;
    ldp->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;

    SetDevicePathNodeLength(ldp, sizeof(*ldp));

    return dp;
}

/*
 * Compare two device paths for equality.
 */
int64_t CompareDevicePaths(const EFI_DEVICE_PATH* dp1, const EFI_DEVICE_PATH* dp2) {
    if (dp1 == NULL || dp2 == NULL) {
        return -1;
    }

    while (TRUE) {
        uint8_t type1 = DevicePathType(dp1);
        uint8_t type2 = DevicePathType(dp2);
        uint8_t subtype1 = DevicePathSubType(dp1);
        uint8_t subtype2 = DevicePathSubType(dp2);
        uint8_t len1 = DevicePathNodeLength(dp1);
        uint8_t len2 = DevicePathNodeLength(dp2);

        if (type1 != type2) {
            return (int64_t)type2 - (int64_t)type1;
        }

        if (subtype1 != subtype2) {
            return (int64_t)subtype1 - (int64_t)subtype2;
        }

        if (len1 != len2) {
            return (int64_t)len1 - (int64_t)len2;
        }

        int64_t ret = CompareMem(dp1, dp2, len1);
        if (ret != 0) {
            return ret;
        }

        if (IsDevicePathEnd(dp1))
            break;

        dp1 = (EFI_DEVICE_PATH*)((char*)dp1 + len1);
        dp2 = (EFI_DEVICE_PATH*)((char*)dp2 + len2);
    }

    return 0;
}

/*
 * Unload an existing file system driver.
 */
EFI_STATUS UnloadDriver(EFI_HANDLE FSHandle) {
    uint64_t OpenInfoCount;
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY* OpenInfo;

    EFI_STATUS Status = uefi_call_wrapper(BS->OpenProtocolInformation, 4, FSHandle, &gEfiDiskIoProtocolGuid, &OpenInfo, &OpenInfoCount);
    if (EFI_ERROR(Status)) {
        return EFI_NOT_FOUND;
    }

    for (UINTN i = 0; i < OpenInfoCount; i++) {
        EFI_DRIVER_BINDING_PROTOCOL* DriverBinding;
        Status = uefi_call_wrapper(BS->OpenProtocol, 6, OpenInfo[i].AgentHandle, &gEfiDriverBindingProtocolGuid, (void**)&DriverBinding, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status)) {
            continue;
        }

        Print(L"Unloading driver with version: 0x%x\n", DriverBinding->Version);
        Status = uefi_call_wrapper(BS->UnloadImage, 1, DriverBinding->ImageHandle);
        if (EFI_ERROR(Status)) {
            Print(L"  Could not unload driver: %r\n", Status);
            continue;
        }

        uefi_call_wrapper(BS->FreePool, 1, OpenInfo);
        return EFI_SUCCESS;
    }

    uefi_call_wrapper(BS->FreePool, 1, OpenInfo);
    return EFI_NOT_FOUND;
}