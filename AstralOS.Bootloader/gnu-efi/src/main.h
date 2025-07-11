#pragma once
#include <efi.h>
#include <efilib.h>
#include <efistdarg.h>
#include <libsmbios.h>

#define SafeFree(p) do { FreePool(p); p = NULL;} while(0)

/*
 * Macro used to compute the size of an array
 */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(Array)   (sizeof(Array) / sizeof((Array)[0]))
#endif

/*
* Macro used to get the current Arch
*/
#if defined(_M_X64) || defined(__x86_64__)
static CHAR16* Arch = L"x64";
static CHAR16* ArchName = L"AMD64";
#elif defined(_M_IX86) || defined(__i386__)
static CHAR16* Arch = L"ia32";
static CHAR16* ArchName = L"AMD32";
#elif defined (_M_ARM64) || defined(__aarch64__)
static CHAR16* Arch = L"aa64";
static CHAR16* ArchName = L"ARM64";
#elif defined (_M_ARM) || defined(__arm__)
static CHAR16* Arch = L"arm";
static CHAR16* ArchName = L"ARM32";
#elif defined(_M_RISCV64) || (defined (__riscv) && (__riscv_xlen == 64))
static CHAR16* Arch = L"riscv64";
static CHAR16* ArchName = L"RISCV64";
#else
#error Unsupported architecture
#endif

#define FILE_INFO_SIZE (PATH_MAX * sizeof(CHAR16))

/* Maximum size to be used for paths */
#ifndef PATH_MAX
#define PATH_MAX            512
#endif

#define STRING_MAX          (PATH_MAX + 2)

static EFI_HANDLE gImageHandle = NULL;

/*
* Framebuffer Struct
*
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-3-Graphics-Output-Protocol/gnu-efi/bootloader/main.c
*/
typedef struct {
    void* BaseAddress;
    size_t BufferSize;
    unsigned int Width;
    unsigned int Height;
    unsigned int PixelsPerScanLine;
} Framebuffer;

typedef struct {
    Framebuffer pFramebuffer;
    EFI_MEMORY_DESCRIPTOR* mMap;
    uint64_t mMapSize;
    uint64_t mMapDescSize;
    void* rsdp;
} BootInfo;

/*
* Get the Drivers Name
*/
static CHAR16* GetDriverName(CONST EFI_HANDLE DriverHandle) {
    CHAR16* DriverName;
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

/*
* Get the Last Device Path
*/
static EFI_DEVICE_PATH* GetLastDevicePath(CONST EFI_DEVICE_PATH_PROTOCOL * dp) {
    EFI_DEVICE_PATH* next, * p;

    if (IsDevicePathEnd(dp))
        return NULL;

    for (p = (EFI_DEVICE_PATH*)dp, next = NextDevicePathNode(p);
        !IsDevicePathEnd(next);
        p = next, next = NextDevicePathNode(next));

    return p;
}

/*
* Get the Parent Device
*/
EFI_DEVICE_PATH* GetParentDevice(CONST EFI_DEVICE_PATH * DevicePath) {
    EFI_DEVICE_PATH* dp, * ldp;

    dp = DuplicateDevicePath((EFI_DEVICE_PATH*)DevicePath);
    if (dp == NULL)
        return NULL;

    ldp = GetLastDevicePath(dp);
    if (ldp == NULL)
        return NULL;

    ldp->Type = END_DEVICE_PATH_TYPE;
    ldp->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;

    SetDevicePathNodeLength(ldp, sizeof(*ldp));

    return dp;
}

#define INITIAL_BUFFER_SIZE 128

/*
* Helper function to format a GUID into a string
*/
EFI_STATUS
FormatGuid(CHAR16 * Buffer, UINTN BufferSize, EFI_GUID * Guid) {
    return SPrint(Buffer, BufferSize, L"%08x-%04x-%04x-%04x-%012lx",
        Guid->Data1,
        Guid->Data2,
        Guid->Data3,
        (Guid->Data4[0] << 8) | Guid->Data4[1],
        *((UINT64*)(&Guid->Data4[2])));
}

/*
* Helper function to append a formatted string to a buffer
*/
EFI_STATUS
AppendToBuffer(CHAR16 * *Buffer, UINTN * BufferSize, UINTN * BufferUsed, CHAR16 * Format, ...) {
    va_list Args;
    UINTN NeededSize;
    CHAR16* TempBuffer;

    va_start(Args, Format);
    NeededSize = VSPrint(NULL, 0, Format, Args) + 1;
    va_end(Args);

    // Resize buffer if necessary
    if (*BufferUsed + NeededSize >= *BufferSize) {
        *BufferSize *= 2;
        TempBuffer = ReallocatePool(*BufferUsed * sizeof(CHAR16), *BufferSize * sizeof(CHAR16), *Buffer);
        if (TempBuffer == NULL) {
            FreePool(*Buffer);
            return EFI_OUT_OF_RESOURCES;
        }
        *Buffer = TempBuffer;
    }

    // Append to the buffer
    va_start(Args, Format);
    *BufferUsed += VSPrint(*Buffer + *BufferUsed, *BufferSize - *BufferUsed, Format, Args);
    va_end(Args);

    return EFI_SUCCESS;
}

/*
* Convert a device path to a text string
*/
CHAR16*
ConvertDevicePathToText(EFI_DEVICE_PATH_PROTOCOL * DevicePath) {
    EFI_DEVICE_PATH_PROTOCOL* Node;
    UINTN BufferSize = INITIAL_BUFFER_SIZE;
    UINTN BufferUsed = 0;
    CHAR16* Buffer;

    if (DevicePath == NULL) {
        return NULL;
    }

    // Allocate initial buffer
    Buffer = AllocateZeroPool(BufferSize * sizeof(CHAR16));
    if (Buffer == NULL) {
        return NULL;
    }

    Node = DevicePath;

    while (!IsDevicePathEnd(Node)) {
        UINT8 Type = DevicePathType(Node);
        UINT8 SubType = DevicePathSubType(Node);

        switch (Type) {
        case HARDWARE_DEVICE_PATH:
            if (SubType == HW_PCI_DP) {
                PCI_DEVICE_PATH* PciNode = (PCI_DEVICE_PATH*)Node;
                AppendToBuffer(&Buffer, &BufferSize, &BufferUsed, L"Pci(0x%x,0x%x)", PciNode->Device, PciNode->Function);
            }
            break;

        case ACPI_DEVICE_PATH:
            if (SubType == ACPI_DP) {
                ACPI_HID_DEVICE_PATH* AcpiNode = (ACPI_HID_DEVICE_PATH*)Node;
                AppendToBuffer(&Buffer, &BufferSize, &BufferUsed, L"PciRoot(0x%x)", AcpiNode->HID);
            }
            break;

        case MESSAGING_DEVICE_PATH:
            if (SubType == MSG_SATA_DP) {
                SATA_DEVICE_PATH* SataNode = (SATA_DEVICE_PATH*)Node;
                AppendToBuffer(&Buffer, &BufferSize, &BufferUsed, L"Sata(0x%x,0x%x,0x%x)", SataNode->HBAPortNumber, SataNode->PortMultiplierPortNumber, SataNode->Lun);
            }
            break;

        case MEDIA_DEVICE_PATH:
            if (SubType == MEDIA_HARDDRIVE_DP) {
                HARDDRIVE_DEVICE_PATH* HdNode = (HARDDRIVE_DEVICE_PATH*)Node;
                CHAR16 GuidBuffer[37];  // Buffer to hold formatted GUID
                if (HdNode->SignatureType == SIGNATURE_TYPE_MBR) {
                    // MBR signature is 4 bytes (first 4 bytes of Signature array)
                    UINT32 MbrSignature;
                    CopyMem(&MbrSignature, HdNode->Signature, sizeof(UINT32));
                    AppendToBuffer(&Buffer, &BufferSize, &BufferUsed, L"HD(%d,MBR,0x%x,0x%lx,0x%lx)",
                        HdNode->PartitionNumber,
                        MbrSignature,
                        HdNode->PartitionStart,
                        HdNode->PartitionSize);
                }
                else if (HdNode->SignatureType == SIGNATURE_TYPE_GUID) {
                    // GPT signature is a 16-byte array (GUID)
                    FormatGuid(GuidBuffer, sizeof(GuidBuffer), (EFI_GUID*)HdNode->Signature);
                    AppendToBuffer(&Buffer, &BufferSize, &BufferUsed, L"HD(%d,GPT,%s,0x%lx,0x%lx)",
                        HdNode->PartitionNumber,
                        GuidBuffer,
                        HdNode->PartitionStart,
                        HdNode->PartitionSize);
                }
            }
            break;

        default:
            AppendToBuffer(&Buffer, &BufferSize, &BufferUsed, L"UnknownPath(Type: 0x%x, SubType: 0x%x)", Type, SubType);
            break;
        }

        // Add '/' separator between nodes
        if (!IsDevicePathEnd(NextDevicePathNode(Node))) {
            AppendToBuffer(&Buffer, &BufferSize, &BufferUsed, L"/");
        }

        // Move to the next node in the device path
        Node = NextDevicePathNode(Node);
    }

    return Buffer;
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
static VOID DisconnectBlockingDrivers(VOID) {
    EFI_STATUS Status;
    UINTN HandleCount = 0, Index, OpenInfoIndex, OpenInfoCount;
    EFI_HANDLE* Handles = NULL;
    CHAR16* DevicePathString;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume;
    EFI_BLOCK_IO_PROTOCOL* BlockIo;
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY* OpenInfo;

    // Get all DiskIo handles
    Status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiDiskIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (EFI_ERROR(Status) || (HandleCount == 0))
        return;

    // Check every DiskIo handle
    for (Index = 0; Index < HandleCount; Index++) {
        // If this is not partition - skip it.
        // This is then whole disk and DiskIo
        // should be opened here BY_DRIVER by Partition driver
        // to produce partition volumes.
        Status = uefi_call_wrapper(BS->OpenProtocol, 6, Handles[Index], &gEfiBlockIoProtocolGuid, (VOID**)&BlockIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

        if (EFI_ERROR(Status))
            continue;
        if ((BlockIo->Media == NULL) || (!BlockIo->Media->LogicalPartition))
            continue;

        // If SimpleFileSystem is already produced - skip it, this is ok
        Status = uefi_call_wrapper(BS->OpenProtocol, 5, Handles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&Volume, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (Status == EFI_SUCCESS)
            continue;

        DevicePathString = ConvertDevicePathToText(DevicePathFromHandle(Handles[Index]));

        // If no SimpleFileSystem on this handle but DiskIo is opened BY_DRIVER
        // then disconnect this connection
        Status = uefi_call_wrapper(BS->OpenProtocolInformation, 4, Handles[Index], &gEfiDiskIoProtocolGuid, &OpenInfo, &OpenInfoCount);
        if (EFI_ERROR(Status)) {
            Print(L"Could not get DiskIo protocol for %s: %r\n", DevicePathString, Status);
            FreePool(DevicePathString);
            continue;
        }

        for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) == EFI_OPEN_PROTOCOL_BY_DRIVER) {
                Status = uefi_call_wrapper(BS->DisconnectController, 3, Handles[Index], OpenInfo[OpenInfoIndex].AgentHandle, NULL);
                if (EFI_ERROR(Status)) {
                    Print(L"Could not disconnect '%s' on %s\n", GetDriverName(OpenInfo[OpenInfoIndex].AgentHandle), DevicePathString);
                }
                else {
                    Print(L"Disconnected '%s' on %s \n", GetDriverName(OpenInfo[OpenInfoIndex].AgentHandle), DevicePathString);
                }
            }
        }
        FreePool(DevicePathString);
        FreePool(OpenInfo);
    }
    FreePool(Handles);
}

/*
 * Compare two device paths for equality.
 */
INTN CompareDevicePaths(CONST EFI_DEVICE_PATH* dp1, CONST EFI_DEVICE_PATH* dp2)
{
    // Check if either of the device paths is NULL
    if (dp1 == NULL || dp2 == NULL)
        return -1;

    while (TRUE) {
        // Get type and subtype of both device paths
        UINT8 type1 = DevicePathType(dp1);
        UINT8 type2 = DevicePathType(dp2);
        UINT8 subtype1 = DevicePathSubType(dp1);
        UINT8 subtype2 = DevicePathSubType(dp2);
        UINTN len1 = DevicePathNodeLength(dp1);
        UINTN len2 = DevicePathNodeLength(dp2);

        // Compare types of both device paths
        if (type1 != type2)
            return (INTN)type2 - (INTN)type1;

        // Compare subtypes of both device paths
        if (subtype1 != subtype2)
            return (INTN)subtype1 - (INTN)subtype2;

        // Compare lengths of both device path nodes
        if (len1 != len2)
            return (INTN)len1 - (INTN)len2;

        // Compare memory of both device path nodes
        INTN ret = CompareMem(dp1, dp2, len1);
        if (ret != 0)
            return ret;

        // If we reach the end of both device paths, exit the loop
        if (IsDevicePathEnd(dp1))
            break;

        // Move to the next node in both device paths
        dp1 = (EFI_DEVICE_PATH*)((char*)dp1 + len1);
        dp2 = (EFI_DEVICE_PATH*)((char*)dp2 + len2);
    }

    // If all nodes are equal, return 0 indicating the device paths are identical
    return 0;
}

/*
* Prints the Buffer in hex format
*/
VOID PrintBuffer(CHAR8* Buffer, UINTN BufferSize) {
    UINTN Index;

    for (Index = 0; Index < BufferSize; Index++) {
        // Print each byte in hex
        Print(L"%02x ", Buffer[Index] & 0xFF);

        // Print a new line after every 16 bytes for readability
        if ((Index + 1) % 16 == 0) {
            Print(L"\n");
        }
    }

    // Print a new line after the buffer dump
    Print(L"\n");
}

/*
 * Unload an existing file system driver.
 */
EFI_STATUS UnloadDriver(EFI_HANDLE FSHandle) {
    EFI_STATUS Status;
    UINTN OpenInfoCount;
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY* OpenInfo;
    EFI_DRIVER_BINDING_PROTOCOL* DriverBinding;

    Status = uefi_call_wrapper(BS->OpenProtocolInformation, 4, FSHandle, &gEfiDiskIoProtocolGuid, &OpenInfo, &OpenInfoCount);
    if (EFI_ERROR(Status)) {
        return EFI_NOT_FOUND;
    }

    for (UINTN i = 0; i < OpenInfoCount; i++) {
        Status = uefi_call_wrapper(BS->OpenProtocol, 6, OpenInfo[i].AgentHandle, &gEfiDriverBindingProtocolGuid, (VOID**)&DriverBinding, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status)) {
            continue;
        }

        // Unload the driver using its image handle
        Print(L"Unloading driver with version: 0x%x\n", DriverBinding->Version);
        Status = uefi_call_wrapper(BS->UnloadImage, 1, DriverBinding->ImageHandle);
        if (EFI_ERROR(Status)) {
            Print(L"  Could not unload driver: %r\n", Status);
            continue;
        }

        // Free the OpenInfo buffer
        uefi_call_wrapper(BS->FreePool, 1, OpenInfo);
        return EFI_SUCCESS;
    }

    // Free the OpenInfo buffer
    uefi_call_wrapper(BS->FreePool, 1, OpenInfo);
    return EFI_NOT_FOUND;
}

/*
* Get the Secure Boot Status
*/
INTN GetSecureBootStatus(VOID) {
    UINT8 SecureBoot = 0, SetupMode = 0;
    UINTN Size;
    INTN SecureBootStatus = 0;

    Size = sizeof(SecureBoot);
    EFI_STATUS Status = uefi_call_wrapper(RT->GetVariable, 5, L"SecureBoot", &gEfiGlobalVariableGuid, NULL, &Size, &SecureBoot);

    if (Status == EFI_SUCCESS) {
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
* Get the Graphics Output Protocol
* 
* Found in https://github.com/Absurdponcho/PonchoOS/blob/Episode-3-Graphics-Output-Protocol/gnu-efi/bootloader/main.c
*/
Framebuffer framebuffer;
Framebuffer* InitializeGOP() {
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_STATUS status;

    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
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