# AstralOS

A Simple AMD 64 UEFI EXT4 Operating System made in C and C++.

![ACPI](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/ACPI.png?raw=true)

## Building
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

## Features
 - Paging
 - GDT
 - IDT
 - Heap Allocator
 - ACPI
 - APIC

## Thanks
OSDev.org
Poncho for the Poncho OS Paging Setup
