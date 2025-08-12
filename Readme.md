# AstralOS

A Simple AMD 64 UEFI EXT4 Operating System made in C and C++.

![ACPI](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/ACPI.png?raw=true)

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

## Testing
If you would like to test the OS. Get the newest release and setup VirtualBox with `Other Linux x64`
<br>
TIP: Make sure you enable UEFI in VBOX.<br>
Unfortunatly, because of Higher Half Mapping, the kernel has gotten a bit slow. It will take a while to initialize.<br>
The kernel needs at least 512 MB of RAM. It is tested on 1 Core on VBox.

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
OSDev.org
Poncho for the Poncho OS Paging Setup
