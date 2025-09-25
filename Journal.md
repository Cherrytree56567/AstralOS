---
Title: "AstralOS"
Author: "@CT5"
Description: "Describe your PCB project in a short sentence!"
Created On: "IDK"
---

# September 25th: AHCI Controller Driver - Part 1

For some reason, VBOX and QEMU don't say that the device is AHCI but instead say that is an IDE Controller, so I switched to QEMU because it had PCIe and the MCFG table would
tell me exactly what type of device each one is. To migrate to QEMU, I needed to fix a few parts of my frankenstein bootloader so that QEMU would run it without issues.
I have written up a whole 62 line comment on how AHCI works, so that others could understand what I am doing. I should be able to finish the AHCI Controller Driver tomorrow.
Here is a screenshot of what I have so far.

![AHCIController1](https://github.com/Cherrytree56567/AstralOS/blob/main/Demos/AHCIController1.png?raw=true)

**Total time spent: 7h 7m**