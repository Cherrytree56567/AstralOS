#pragma once
#include "main.h"

/*
 * Macro used to compute the size of an array
 */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(Array)   (sizeof(Array) / sizeof((Array)[0]))
#endif

#define FILE_INFO_SIZE (PATH_MAX * sizeof(CHAR16))

/* Maximum size to be used for paths */
#ifndef PATH_MAX
#define PATH_MAX            512
#endif

#define STRING_MAX          (PATH_MAX + 2)

#define INITIAL_BUFFER_SIZE 128

static Framebuffer framebuffer;

void* FindRSDP(EFI_SYSTEM_TABLE* SystemTable);
int64_t GetSecureBootStatus();
void DisconnectBlockingDrivers();
Framebuffer* InitializeGOP();
uint16_t* GetDriverName(const EFI_HANDLE DriverHandle);
EFI_DEVICE_PATH* GetLastDevicePath(const EFI_DEVICE_PATH_PROTOCOL* dp);
EFI_DEVICE_PATH* GetParentDevice(const EFI_DEVICE_PATH* DevicePath);
int64_t CompareDevicePaths(const EFI_DEVICE_PATH* dp1, const EFI_DEVICE_PATH* dp2);
EFI_STATUS UnloadDriver(EFI_HANDLE FSHandle);