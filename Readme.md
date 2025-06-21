# AstralOS

A Simple AMD 64 UEFI EXT4 Operating System made in C and C++.

## Building
To build on Windows, install WSL (Windows Subsystem For Linux) and upgrade your distro to WSL 2.
Then initialize cmake by running:
```
cmake -B build -G Ninja
```
To install Dependencies and setup directories run:
```
cmake --build build --target install
```

Then Build the Bootloader and Kernel by running:
```
cmake --build build --target build
```

## Features
 - Paging

## Thanks
OSDev.org
Poncho for the Poncho OS Paging Setup