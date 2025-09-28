#pragma once
#include "../main.h"

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

#define INITIAL_BUFFER_SIZE 128

EFI_GUID Acpi20TableGuid = { 0x8868e871, 0xe4f1, 0x11d3, {0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81} };

EFI_GUID Acpi10TableGuid = {
    0xeb9d2d30, 0x2d88, 0x11d3,
    { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d }
};

CHAR16* GetDriverName(CONST EFI_HANDLE DriverHandle);
EFI_DEVICE_PATH* GetLastDevicePath(CONST EFI_DEVICE_PATH_PROTOCOL * dp);
EFI_DEVICE_PATH* GetParentDevice(CONST EFI_DEVICE_PATH * DevicePath);
EFI_STATUS AppendToBuffer(CHAR16 * *Buffer, UINTN * BufferSize, UINTN * BufferUsed, CHAR16 * Format, ...);
CHAR16* ConvertDevicePathToText(EFI_DEVICE_PATH_PROTOCOL * DevicePath);
INTN CompareDevicePaths(CONST EFI_DEVICE_PATH* dp1, CONST EFI_DEVICE_PATH* dp2);
VOID PrintBuffer(CHAR8* Buffer, UINTN BufferSize);
EFI_STATUS UnloadDriver(EFI_HANDLE FSHandle);

Framebuffer framebuffer;

void* FindRSDP(EFI_SYSTEM_TABLE* SystemTable);
int64_t GetSecureBootStatus();
void DisconnectBlockingDrivers();
Framebuffer* InitializeGOP();