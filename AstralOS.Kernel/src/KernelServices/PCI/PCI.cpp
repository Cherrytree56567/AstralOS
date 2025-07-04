#include "PCI.h"

/*
 * The PCI wiki on the OSDev website
 * is a bit hard to understand, so I
 * asked ChatGPT for this part. ik, 
 * ik, but this isn't vibe coding.
 * 
 * The PCI Configuration Space Access
 * Mechanism #1 is an I/O Port which
 * are 0xCF8 and 0xCFC respectively.
 * 
 * You can write to 0xCF8 which is
 * the CONFIG_ADDRESS to specify what
 * you want to access.
 * 
 * Then you can access 0xCFC which is
 * CONFIG_DATA to access the data you
 * selected.
 * 
 * --CONFIG ADDRESS REGISTER--
 * Bit 31 is the Enable Bit.
 * Bits 30 to 24 is Reserved.
 * Bits 23 to 16 which is the Bus Num.
 * You can use this to choose which
 * PCI bus you want to look at.
 * Bits 15 to 11 is where you specify
 * the Device Number.
 * Bits 10 to 8 is where you specify
 * the which function of the device
 * you want to use.
 * Bits 7 to 0 is the Register offset.
 * You can use this to choose the
 * config register inside the function.
 * 
 * TIP: Bits 1 to 0 of offset must be 0 
 * because you can only read/write 32-bit 
 * chunks (4 bytes at a time). So offset 
 * goes in steps of 4: 0x00, 0x04, 0x08... 
 * up to 0xFC.
 * 
 * There are other ways to config the PCI
 * too like the Configuration Space Access 
 * Mechanism #2. This method was deprecated 
 * in PCI version 2.0, so it's unlikely you
 * would see it now.
 * 
 * For access mechanism #2, the IO port at 
 * 0xCF8 is an 8-bit port and is used to 
 * enable/disable the access mechanism and 
 * set the function number. 
 * 
 * Bits 7-0: Key (0 = access mechanism disabled, non-zero = access mechanism enabled) 
 * Bits 3-1: Function number
 * Bit 0: Special cycle enabled if set
 * 
 * The IO port at 0xCFA is also an 8-bit 
 * port, and is used to set the bus number 
 * for subsequent PCI configuration space 
 * accesses.
 * Once the access mechanism has been enabled; 
 * accesses to IO ports 0xC000 to 0xCFFF are 
 * used to access PCI configuration space.
 * 
 * Bits 15-12: Must be 1100b
 * Bits 11-8: Device Number
 * Bits 7-2: Register index
 * Bits 1-0: Must be zero
 * 
 * Note that this limits the system to 16 devices per PCI bus. 
*/
uint16_t PCI::ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
  
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
  
    outl(0xCF8, address);

    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

/*
 * We can Test if PCI Exists by
 * reading all 32 devices because
 * PCI only allows 32 devices per
 * bus.
 * 
 * This won't be able to check if
 * PCIe is supported but it will
 * check if PCI is supported.
 * Usually both are supported.
*/
bool PCI::PCIExists() {
    for (uint8_t device = 0; device < 32; device++) {
        uint16_t vendor = ConfigReadWord(0, device, 0, 0x00) & 0xFFFF;
        if (vendor != 0xFFFF) {
            return true;
        }
    }
    return false;
}

/*
 * If set to 1 the device can respond to I/O Space accesses; otherwise, the device's response is disabled.
*/
void PCI_Header::SetIOSpace(bool val) {
    if (val) {
        Command |= (1 << 0);
    } else {
        Command &= ~(1 << 0);
    }
}

bool PCI_Header::IOSpace()  {
    return Command & (1 << 0);
}

/*
 * If set to 1 the device can respond to Memory Space accesses; otherwise, the device's response is disabled.
*/
void PCI_Header::SetMemorySpace(bool val) {
    if (val) {
        Command |= (1 << 1);
    } else {
        Command &= ~(1 << 1);
    }
}

bool PCI_Header::MemorySpace() {
    return Command & (1 << 1);
}

/*
 * If set to 1 the device can behave as a bus master; otherwise, the device can not generate PCI accesses.
*/
void PCI_Header::SetBusMaster(bool val) {
    if (val) {
        Command |= (1 << 2);
    } else {
        Command &= ~(1 << 2);
    }
}

bool PCI_Header::BusMaster() {
    return Command & (1 << 2);
}

/*
* If set to 1 the device can monitor Special Cycle operations; otherwise, the device will ignore them.
*/
bool PCI_Header::SpecialCycles() {
    return Command & (1 << 3);
}

/*
 * If set to 1 the device can generate the Memory Write and Invalidate command; otherwise, the Memory Write command must be used.
*/
bool PCI_Header::MemWrite() {
    return Command & (1 << 4);
}

/*
 * If set to 1 the device does not respond to palette register writes and will snoop the data; 
 * otherwise, the device will trate palette write accesses like all other accesses.
*/
bool PCI_Header::VGAPaletteSnoop() {
    return Command & (1 << 5);
}

/*
 * If set to 1 the device will take its normal action when a parity error
 * is detected; otherwise, when an error is detected, the device will set 
 * bit 15 of the Status register (Detected Parity Error Status Bit), but 
 * will not assert the PERR# (Parity Error) pin and will continue operation 
 * as normal.
*/
void PCI_Header::SetParityErrorResponse(bool val) {
    if (val) {
        Command |= (1 << 6);
    } else {
        Command &= ~(1 << 6);
    }
}

bool PCI_Header::ParityErrorResponse() {
    return Command & (1 << 6);
}

/*
 * If set to 1 the SERR# driver is enabled; otherwise, the driver is disabled.
*/
void PCI_Header::SetSERREnable(bool val) {
    if (val) {
        Command |= (1 << 8);
    } else {
        Command &= ~(1 << 8);
    }
}

bool PCI_Header::SERREnable() {
    return Command & (1 << 8);
}

/*
 * If set to 1 indicates a device is allowed to generate fast back-to-back transactions; otherwise, fast back-to-back transactions are only allowed to the same agent.
*/
bool PCI_Header::FastBBEnable() {
    return Command & (1 << 9);
}

/*
 * If set to 1 the assertion of the devices INTx# signal is disabled; otherwise, assertion of the signal is enabled.
*/
void PCI_Header::SetInterruptDisable(bool val) {
    if (val) {
        Command |= (1 << 10);
    } else {
        Command &= ~(1 << 10);
    }
}

bool PCI_Header::InterruptDisable() {
    return Command & (1 << 10);
}

/*
 * Represents the state of the device's INTx# signal. 
 * If set to 1 and bit 10 of the Command register 
 * is set to 0 the signal will be asserted; otherwise, 
 * the signal will be ignored.
*/
bool PCI_Header::InterruptStatus() {
    return Command & (1 << 3);
}

/*
 * If set to 1 the device implements the pointer for a 
 * New Capabilities Linked list at offset 0x34; otherwise, 
 * the linked list is not available.
*/
bool PCI_Header::CapabilitiesList() {
    return Command & (1 << 4);
}

/*
 * If set to 1 the device is capable of running at 66 MHz; otherwise, the device runs at 33 MHz.
*/
bool PCI_Header::MHzCapable() {
    return Command & (1 << 5);
}

/*
 * If set to 1 the device can accept fast back-to-back 
 * transactions that are not from the same agent; otherwise, 
 * transactions can only be accepted from the same agent.
*/
bool PCI_Header::FastBBCapable() {
    return Command & (1 << 7);
}

/*
 * This bit is only set when the following conditions are met. 
 * The bus agent asserted PERR# on a read or observed an assertion 
 * of PERR# on a write, the agent setting the bit acted as the bus 
 * master for the operation in which the error occurred, and bit 6 
 * of the Command register (Parity Error Response bit) is set to 1.
*/
void PCI_Header::SetMasterDataParityError() {
    Status |= (1 << 8);
}

bool PCI_Header::MasterDataParityError() {
    return Command & (1 << 8);
}

/*
 * Read only bits that represent the slowest time that a device will 
 * assert DEVSEL# for any bus command except Configuration Space read 
 * and writes. Where a value of 0x0 represents fast timing, a value 
 * of 0x1 represents medium timing, and a value of 0x2 represents 
 * slow timing.
*/
DEVSEL PCI_Header::DEVSELTiming() {
    return static_cast<DEVSEL>((Status >> 9) & 0b11);
}

/*
 * This bit will be set to 1 whenever a target device terminates a 
 * transaction with Target-Abort.
*/
void PCI_Header::SetSignaledTargetAbort() {
    Status |= (1 << 11);
}

bool PCI_Header::SignaledTargetAbort() {
    return Command & (1 << 11);
}

/*
 * This bit will be set to 1, by a master device, whenever its 
 * transaction is terminated with Target-Abort.
*/
void PCI_Header::SetRecievedTargetAbort() {
    Status |= (1 << 12);
}

bool PCI_Header::RecievedTargetAbort() {
    return Command & (1 << 12);
}

/*
 * This bit will be set to 1, by a master device, whenever its 
 * transaction (except for Special Cycle transactions) is terminated 
 * with Master-Abort.
*/
void PCI_Header::SetRecievedMasterAbort() {
    Status |= (1 << 13);
}

bool PCI_Header::RecievedMasterAbort() {
    return Command & (1 << 13);
}

/*
 * This bit will be set to 1 whenever the device asserts SERR#.
*/
void PCI_Header::SetSignaledSystemError() {
    Status |= (1 << 14);
}

bool PCI_Header::SignaledSystemError() {
    return Command & (1 << 14);
}

/*
 * This bit will be set to 1 whenever the device detects a parity 
 * error, even if parity error handling is disabled.
*/
bool PCI_Header::DetectedParityError() {
    return Command & (1 << 15);
}

void PCI_Header::SetDetectedParityError() {
    Status |= (1 << 15);
}

PCIHeaderType PCI_Header::GetHeaderType() {
    return static_cast<PCIHeaderType>(HeaderType & 0x7F);
}

bool PCI_Header::IsMultiFunction() {
    return (HeaderType & 0x80) != 0;
}

/*
 * Will return 0, after BIST execution, if the test completed successfully.
*/
uint8_t PCI_Header::GetCompletionCode() {
    return BIST & 0x0F;
}

/*
 * When set to 1 the BIST is invoked. 
 * This bit is reset when BIST completes. 
 * If BIST does not complete after 2 seconds the device should be failed by system software.
*/
void PCI_Header::SetStartBIST(bool start) {
    if (start) {
        BIST |= (1 << 6);
    } else {
        BIST &= ~(1 << 6);
    }
}

bool PCI_Header::IsStartBIST() {
    return (BIST >> 6) & 0x1;
}

/*
 * Will return 1 the device supports BIST.
*/
bool PCI_Header::IsBISTCapable() {
    return (BIST >> 7) & 0x1;
}