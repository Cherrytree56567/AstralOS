---
Title: "AstralOS"
Author: "@CT5"
Description: "A simple OS made in C++. It includes a lot of features, like a hybrid kernel (eventually) and a multi partition os. The code contains comments showing how everything works and which parts are from other sources."
Created On: "IDK"
---

# September 8-10th: Reorganisation

I didn't do a lot today, but I was able to reorganise my messy code. I was able to create a String Class that allows you to easily cat strings without 10+ lines of code. I was also able to add a Driver Registration function in Driver Services for the Driver to register a Base Driver. I just need to figure out how Im going to make the Driver.

![strcat](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/strcat.png?raw=true)

**Total time spent: 40m**

# September 19-21th: Dynamic ELF Parser

I was finally able to get the Driver working. Turns out it was due to an error in the ELF Parser. I added a dynamic elf parser and a relocator so that it executes the driver correctly.

![DriverFix](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/DriverFix.png?raw=true)

**Total time spent: 10h 13m**

# September 22th: IDE Controller - Part 1

I was able to fix the Driver Issue, turns out it was because it was because I passed DriverServices Incorrectly. I was able to fix it but then I realised that VBOX doesn't expose AHCI stuff in PCI so I'd have to use the IDE mode. So now Im making an IDE Controller.

![IDEController](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/IDEController.png?raw=true)

**Total time spent: 2h 19m**

# September 23-24th: IDE Controller - Part 2

I added an PIT class for the APIC timer so that I could test the sleep functionality and use sleep for waiting in the IDE Controller. The example shows the IDE Driver determining weather a drive is present or not and reading the drive's buffer from LBA 0.

![IDEController1](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/IDEController1.png?raw=true)

**Total time spent: 6h 33m**

# September 25th: AHCI Controller Driver - Part 1

For some reason, VBOX and QEMU don't say that the device is AHCI but instead say that is an IDE Controller, so I switched to QEMU because it had PCIe and the MCFG table would
tell me exactly what type of device each one is. To migrate to QEMU, I needed to fix a few parts of my frankenstein bootloader so that QEMU would run it without issues.
I have written up a whole 62 line comment on how AHCI works, so that others could understand what I am doing. I should be able to finish the AHCI Controller Driver tomorrow.
Here is a screenshot of what I have so far.

![AHCIController1](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/AHCIController1.png?raw=true)

**Total time spent: 7h 7m**

# September 26th: AHCI Controller Driver - Part 2

I fixed a few errors that I had before which were really really difficult to debug. The AHCI Driver was really hard to debug because there wasn't enough viable information online or on the osdev wiki. The OSDev Wiki just gave me a few pointers and a checklist to follow. I found a lot of the defines from online documentation from different websites, so you'll see a lot of sourcing there. I was able to also read a bunch of sectors. I did face an issue with the IDE Driver which I had here, which was that reading sectors wouldn't work and would give nothing in the buffers. But I found this good [forum](https://forum.osdev.org/viewtopic.php?t=57022&sid=d0273e898faa4fafffb520174a5f2f10&start=15) on the OSDev Wiki that helped a lot.

![AHCIController2](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/AHCIController2.png?raw=true)

**Total time spent: 6h 35m**

# September 27-28th: Bootloader Revamp

I haven't finished revamping the bootloader yet. But here is what I have so far. I decided to revamp it because it was mainly someone else's code and was horribly formatted.

![AllocPages](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/AllocPages.png?raw=true)

**Total time spent: 10h 10m**

# September 29th: Added GPT Partitioning

Today I was working on the bootloader thing from yesterday. I wanted to fix my messy frankenstein bootloader to work well and not hardcode values and stuff. After a long debugging session, I realised it was not worth it. I was thinking of switching to GRUB but I would have to revamp my kernel for that too. Then I tried to revamp my Driver System because before it would just work for PCI/PCIe Devices but not for other types of drivers like the partition driver I was working on. Once that was done, I worked on making a GPT Partition reader. It wasn't too hard, but it was a challenge.

![GPTPartition](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/GPTPartition.png?raw=true)

**Total time spent: 9h 17m**

# September 30th - October 1st: Fixed bugs in the Web Demo

The web demo thing was hard to patch, because the build system had a lot of errors. I was finally able to patch it, even if it wouldn't last long. The main thing I needed to patch was the `tcg.c` file which had a define that defined how much memory it had for some type of buffer. I was able to raise the limit but I still needed to fix the build system. meson complained that it couldn't run the wasm files, so I just changed the `exe_wrapper` to `true` so that whatever it was passed, it would never fail. This worked, but then chrome complained about some error it couldn't display because I didn't add the assertions flag. But once I did, it gave me the error which told me to enable LZ4 support which I did. Then I fixed a bunch of UI stuff. I definitely worked more than 1h, but that is what Wakatime Tracked.

![Web](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/Web.png?raw=true)

**Total time spent: 1h 44m**

# October 2nd - October 1st: Web Demo Cool Bg

I thought the demo was a bit too simple, so I added this cool react bits background to it that fit in with the theme of the os.

![Web1](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/Web1.png?raw=true)

**Total time spent: 1h 50m 11s**

# December 17th: Added Find Dir Func

Currently I've been working on the EXT4 Driver, one function at a time. Im still working on the CreateDir function, but here is the FindDir (Find in Directory) function I made.

![FindDir](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/FindDir.png?raw=true)

**Total time spent: 2h**

# December 18th: Added Create Dir Func - Part 1

Ive gotten the Create Dir Func to work but it is very very buggy and Ill probably fix it tmrw. You can't even boot the image after it modifies the fs once.

![CreateDir](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/CreateDir.png?raw=true)

**Total time spent: 2h 6m**

# December 19th: CreateDir Func is Cooked

tbh I can't even bother to get this function working, and there is barely any info online or in the OSDev Wiki to understand how to create a dir in EXT4. Right now, it just creates a dir, but if you go inside, there are 2 of the dirs you just created and all the dirs in the parent.

![CreateDir2](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/CreateDir2.png?raw=true)

**Total time spent: 2h 26m**

# December 20 - 23rd: Added Checksuming and Fixed Bugs

I was following the OSDev Wiki before, but it only had stuff on EXT2 which was vastly different compared to EXT4. I underestimated how different those two filesystems were which caused a lot of issues. Today (23rd) I discovered that the Inode struct I was using was wrong for EXT4, so I found this nice [Gitlab Page](https://gitlab.incoresemi.com/software/linux/-/blob/0e698dfa282211e414076f9dc7e83c1c288314fd/Documentation/filesystems/ext4/inodes.rst#i-osd2) which explained how EXT4 works and the structs that were involved. Unfortunatly it didn't tell me how to checksum things which was a big issue because `fsck` was complaining about checksums, so I had to look at the Linux Kernel Source. I still havent gotten to checksuming the Group Descriptors. I was able to checksum the superblock though. With the help of `debugfs` using this command `sudo debugfs /dev/nbd0p2` I was able to identify issues with the Inode Table.

![InodeIssues](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/InodeIssues.png?raw=true)

**Total time spent: 16h 58m**

# December 24th: Finished CreateDir and Started Organising

The cool easter egg value I was putting into a reserved EXT4 OSVal2 var caused the whole thing to fail and caused some weird issues like corrupted creation time and corrupted ExtraBlocks. After I fixed that I realised that the InodeType Enums were wrong so I found the correct one from the [Gitlab Page](https://gitlab.incoresemi.com/software/linux/-/blob/0e698dfa282211e414076f9dc7e83c1c288314fd/Documentation/filesystems/ext4/inodes.rst#i-osd2) which helped me to enable Extents in my inode. After that I tested it with `debugfs` and it worked! Although List Dir Still doesnt work properly.

![CreateDir3](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/CreateDir3.png?raw=true)

**Total time spent: 2h 28m**