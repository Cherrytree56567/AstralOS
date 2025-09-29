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

# September 27-28th: XX


![AHCIController2](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/AHCIController2.png?raw=true)

**Total time spent: Xh Xm**

# September 29th: Added GPT Partitioning

Today I was working on the bootloader thing from yesterday. I wanted to fix my messy frankenstein bootloader to work well and not hardcode values and stuff. After a long debugging session, I realised it was not worth it. I was thinking of switching to GRUB but I would have to revamp my kernel for that too. Then I tried to revamp my Driver System because before it would just work for PCI/PCIe Devices but not for other types of drivers like the partition driver I was working on. Once that was done, I worked on making a GPT Partition reader. It wasn't too hard, but it was a challenge.

![GPTPartition](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/GPTPartition.png?raw=true)

**Total time spent: 6h 47m**