# AstralOS

A Simple AMD 64 UEFI EXT4 Operating System made in C and C++.

![Banner](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/Banner.jpg?raw=true)

## Building
If you are using WSL, make sure you use the build with NBD. There is an msi in the root dir with NBD.
To build on Windows, install WSL (Windows Subsystem For Linux) and upgrade your distro to WSL 2.
Then initialize cmake by running:
```
cmake -B .
```
To install Dependencies and setup directories run:
```
cmake --build . --target init
```

Then Build the Bootloader and Kernel by running:
```
cmake --build . --target build
```

To test the build just run 
```
cmake --build . --target run
```

## Testing
If you would like to test the OS. Get the newest release, clone the repo and put the Image in AstralOS.Testing. Then just go inside AstralOS.Testing and run:
```
sudo qemu-system-x86_64 -machine q35 -cpu qemu64 -m 4G -drive file=AstralOS.qcow2 -drive if=pflash,format=raw,unit=0,file="../OVMFbin/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="../OVMFbin/OVMF_VARS-pure-efi.fd" -net none
```

## Features
 - Paging
 - GDT
 - IDT
 - Heap Allocator
 - ACPI
 - APIC
 - PCI
 - PCIe (UNTESTED)

## Thanks
[OSDev Wiki](https://wiki.osdev.org/Expanded_Main_Page)<br>
[Poncho for the Poncho OS Paging Setup](https://github.com/Absurdponcho/PonchoOS)<br>
[Android Bionic LibC](https://android.googlesource.com/platform/bionic/+/ics-mr0/libc/string)<br>
[UEFI:NTFS](https://github.com/pbatard/uefi-ntfs)