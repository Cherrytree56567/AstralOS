#include "PCI.h"
#include "../KernelServices.h"

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
 * We can use this to quickly get the device class
*/
uint32_t PCI::GetDeviceClass(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF) {
    return (static_cast<uint32_t>(ClassCode) << 24) |
           (static_cast<uint32_t>(SubClass)  << 16) |
           (static_cast<uint32_t>(ProgIF)    << 8);
}

/*
 * We can use this to get the class code
 * as a string.
*/
char* PCI::GetClassCode(uint8_t ClassCode) {
    switch (static_cast<ClassCodes>(ClassCode)) {
        case ClassCodes::Unclassified:
            return "Unclassified";
            break;
        case ClassCodes::MassStorageController:
            return "Mass Storage Controller";
            break;
        case ClassCodes::NetworkController:
            return "Network Controller";
            break;
        case ClassCodes::DisplayController:
            return "Display Controller";
            break;
        case ClassCodes::MultimediaController:
            return "Multimedia Controller";
            break; 
        case ClassCodes::MemoryController:
            return "Memory Controller";
            break;
        case ClassCodes::Bridge:
            return "Bridge";
            break;
        case ClassCodes::SimpleCommunicationController:
            return "Simple Communication Controller";
            break;
        case ClassCodes::BaseSystemPeripheral:
            return "Base System Peripheral";
            break;
        case ClassCodes::InputDeviceController:
            return "Input Device Controller";
            break;
        case ClassCodes::DockingStation:
            return "Docking Station";
            break;
        case ClassCodes::Processor:
            return "Processor";
            break;
        case ClassCodes::SerialBusController:
            return "Serial Bus Controller";
            break;
        case ClassCodes::WirelessController:
            return "Wireless Controller";
            break;
        case ClassCodes::IntelligentController:
            return "Intelligent Controller";
            break;
        case ClassCodes::SatelliteCommunication:
            return "Satellite Communication Controller";
            break;
        case ClassCodes::EncryptionController:
            return "Encryption Controller";
            break;
        case ClassCodes::SignalProcessingController:
            return "Signal Processing Controller";
            break;
        case ClassCodes::ProcessingAccelerator:
            return "Processing Accelerator";
            break;
        default:
            return "Unknown Class Code";
            break;
    }
}

char* PCI::GetDeviceCode(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF) {
    switch (static_cast<ClassCodes>(ClassCode)) {
        case ClassCodes::Unclassified: {
            switch (static_cast<UnclassifiedSubClass>(SubClass)) {
                case UnclassifiedSubClass::Non_VGACompatible:
                    return "Unclassified, Non-VGA Compatible";
                case UnclassifiedSubClass::VGACompatible:
                    return "Unclassified, VGA Compatible";
                default:
                    return "Unclassified, Unknown";
            }
        }
        case ClassCodes::MassStorageController: {
            switch (static_cast<MassStorageControllerSubClass>(SubClass)) {
                case MassStorageControllerSubClass::SCSIBusController:
                    return "Mass Storage Controller, SCSI";
                    break;
                case MassStorageControllerSubClass::IDEController: {
                    switch (static_cast<IDEControllerProgIF>(ProgIF)) {
                        case IDEControllerProgIF::ISACompatibilityModeOnlyController:
                            return "Mass Storage Controller, IDE, ISA Compatibility Mode Only";
                        case IDEControllerProgIF::PCINativeModeOnlyController:
                            return "Mass Storage Controller, IDE, PCI Native Mode Only";
                        case IDEControllerProgIF::ISACompatibilityModeController:
                            return "Mass Storage Controller, IDE, ISA Compatibility Mode";
                        case IDEControllerProgIF::PCINativeModeController:
                            return "Mass Storage Controller, IDE, PCI Native Mode";
                        case IDEControllerProgIF::ISACompatibilityModeOnlyControllerBusMastering:
                            return "Mass Storage Controller, IDE, ISA Compatibility Mode Only - Bus Mastering";
                        case IDEControllerProgIF::PCINativeModeOnlyControllerBusMastering:
                            return "Mass Storage Controller, IDE, PCI Native Mode Only - Bus Mastering";
                        case IDEControllerProgIF::ISACompatibilityModeControllerBusMastering:
                            return "Mass Storage Controller, IDE, ISA Compatibility Mode - Bus Mastering";
                        case IDEControllerProgIF::PCINativeModeControllerBusMastering:
                            return "Mass Storage Controller, IDE, PCI Native Mode - Bus Mastering";
                        default:
                            return "Mass Storage Controller, IDE, Unknown";
                    }
                    break;
                }
                case MassStorageControllerSubClass::FloppyDiskController:
                    return "Mass Storage Controller, Floppy Disk";
                    break;
                case MassStorageControllerSubClass::IPIBusController:
                    return "Mass Storage Controller, IPI";
                    break;
                case MassStorageControllerSubClass::RAIDController:
                    return "Mass Storage Controller, RAID";
                    break;
                case MassStorageControllerSubClass::ATAController: {
                    switch (static_cast<ATAControllerProgIF>(ProgIF)) {
                        case ATAControllerProgIF::SingleDMA:
                            return "Mass Storage Controller, ATA, Single DMA";
                        case ATAControllerProgIF::ChainedDMA:
                            return "Mass Storage Controller, ATA, Chained DMA";
                        default:
                            return "Mass Storage Controller, ATA, Unknown";
                    }
                    break;
                }
                case MassStorageControllerSubClass::SerialATAController: {
                    switch (static_cast<SATAControllerProgIF>(ProgIF)) {
                        case SATAControllerProgIF::VendorSpecificInterface:
                            return "Mass Storage Controller, Serial ATA, Vendor Specific Interface";
                        case SATAControllerProgIF::ACHI1_0:
                            return "Mass Storage Controller, Serial ATA, ACHI 1.0";
                        case SATAControllerProgIF::SerialStorageBus:
                            return "Mass Storage Controller, Serial ATA, Serial Storage Bus";
                        default:
                            return "Mass Storage Controller, Serial ATA, Unknown";
                    }
                    break;
                }
                case MassStorageControllerSubClass::SerialAttachedSCSIController: {
                    switch (static_cast<SerialAttachedSCSIControllerProgIF>(ProgIF)) {
                        case SerialAttachedSCSIControllerProgIF::SAS:
                            return "Mass Storage Controller, Serial Attached SCSI";
                        case SerialAttachedSCSIControllerProgIF::SerialStorageBus:
                            return "Mass Storage Controller, Serial Attached SCSI, Serial Storage Bus";
                        default:
                            return "Mass Storage Controller, Serial Attached SCSI, Unknown";
                    }
                    break;
                }
                case MassStorageControllerSubClass::Non_VolatileMemoryController: {
                    switch (static_cast<NonVolatileMemoryControllerProgIF>(ProgIF)) {
                        case NonVolatileMemoryControllerProgIF::NVMHCI:
                            return "Mass Storage Controller, Non-Volatile Memory, NVMHCI";
                        case NonVolatileMemoryControllerProgIF::NVMExpress:
                            return "Mass Storage Controller, Non-Volatile Memory, NVM Express";
                        default:
                            return "Mass Storage Controller, Non-Volatile Memory, Unknown";
                    }
                    break;
                }
                case MassStorageControllerSubClass::Other:
                    return "Mass Storage Controller, Other";
                    break;
                default:
                    return "Mass Storage Controller, Unknown";
            }
            break;
        }
        case ClassCodes::NetworkController: {
            switch (static_cast<NetworkControllerSubClass>(SubClass)) {
                case NetworkControllerSubClass::EthernetController:
                    return "Network Controller, Ethernet";
                    break;
                case NetworkControllerSubClass::TokenRingController:
                    return "Network Controller, Token Ring";
                    break;
                case NetworkControllerSubClass::FDDIController:
                    return "Network Controller, FDDI";
                    break;
                case NetworkControllerSubClass::ATMController:
                    return "Network Controller, ATM";
                    break;
                case NetworkControllerSubClass::ISDNController:
                    return "Network Controller, ISDN";
                    break;
                case NetworkControllerSubClass::WorldFipController:
                    return "Network Controller, WorldFIP";
                    break;
                case NetworkControllerSubClass::PICMGController:
                    return "Network Controller, PICMG";
                    break;
                case NetworkControllerSubClass::InfinibandController:
                    return "Network Controller, Infiniband";
                    break;
                case NetworkControllerSubClass::FabricController:
                    return "Network Controller, Fabric";
                    break;
                case NetworkControllerSubClass::Other:
                    return "Network Controller, Other";
                    break;
                default:
                    return "Network Controller, Unknown";
            }
            break;
        }
        case ClassCodes::DisplayController: {
            switch (static_cast<DisplayControllerSubClass>(SubClass)) {
                case DisplayControllerSubClass::VGACompatibleController: {
                    switch (static_cast<VGACompatibleControllerProgIF>(ProgIF)) {
                        case VGACompatibleControllerProgIF::VGAController:
                            return "Display Controller, VGA";
                        case VGACompatibleControllerProgIF::_8514CompatibleController:
                            return "Display Controller, 8514 Compatible";
                        default:
                            return "Display Controller, VGA, Unknown";
                    }
                    break;
                }
                case DisplayControllerSubClass::XGAController:
                    return "Display Controller, XGA";
                    break;
                case DisplayControllerSubClass::_3DController:
                    return "Display Controller, 3D";
                    break;
                case DisplayControllerSubClass::Other:
                    return "Display Controller, Other";
                    break;
                default:
                    return "Display Controller, Unknown";
            }
            break;
        }
        case ClassCodes::MultimediaController: {
            switch (static_cast<MultimediaControllerSubClass>(SubClass)) {
                case MultimediaControllerSubClass::MultimediaVideoController:
                    return "Multimedia Controller, Video";
                    break;
                case MultimediaControllerSubClass::MultimediaAudioController:
                    return "Multimedia Controller, Audio";
                    break;
                case MultimediaControllerSubClass::ComputerTelephonyDevice:
                    return "Multimedia Controller, Computer Telephony Device";
                    break;
                case MultimediaControllerSubClass::AudioDevice:
                    return "Multimedia Controller, Audio Device";
                    break;
                case MultimediaControllerSubClass::Other:
                    return "Multimedia Controller, Other";
                    break;
                default:
                    return "Multimedia Controller, Unknown";
            }
            break;
        }
        case ClassCodes::MemoryController: {
            switch (static_cast<MemoryControllerSubClass>(SubClass)) {
                case MemoryControllerSubClass::RAMController:
                    return "Memory Controller, RAM";
                    break;
                case MemoryControllerSubClass::FlashController:
                    return "Memory Controller, Flash";
                    break;
                case MemoryControllerSubClass::Other:
                    return "Memory Controller, Other";
                    break;
                default:
                    return "Memory Controller, Unknown";
            }
            break;
        }
        case ClassCodes::Bridge: {
            switch (static_cast<BridgeSubClass>(SubClass)) {
                case BridgeSubClass::HostBridge:
                    return "Bridge, Host";
                    break;
                case BridgeSubClass::ISABridge:
                    return "Bridge, ISA";
                    break;
                case BridgeSubClass::EISABridge:
                    return "Bridge, EISA";
                    break;
                case BridgeSubClass::MCABridge:
                    return "Bridge, MCA";
                    break;
                case BridgeSubClass::PCItoPCI: {
                    switch (static_cast<PCIPCIBridgeProgIF>(ProgIF)) {
                        case PCIPCIBridgeProgIF::NormalDecode:
                            return "Bridge, PCI to PCI, Normal Decode";
                        case PCIPCIBridgeProgIF::SubtractiveDecode:
                            return "Bridge, PCI to PCI, Subtractive Decode";
                        default:
                            return "Bridge, PCI to PCI, Unknown";
                    }
                    break;
                }
                case BridgeSubClass::PCMCIABridge:
                    return "Bridge, PCMCIA";
                    break;
                case BridgeSubClass::NuBusBridge:
                    return "Bridge, NuBus";
                    break;
                case BridgeSubClass::CardBusBridge:
                    return "Bridge, CardBus";
                    break;
                case BridgeSubClass::RACEwayBridge: {
                    switch (static_cast<RACEwayBridgeProgIF>(ProgIF)) {
                        case RACEwayBridgeProgIF::TransparantMode:
                            return "Bridge, RACEway, Transparent Mode";
                        case RACEwayBridgeProgIF::NonTransparentMode:
                            return "Bridge, RACEway, Non-Transparent Mode";
                        default:
                            return "Bridge, RACEway, Unknown";
                    }
                    break;
                }
                case BridgeSubClass::_PCItoPCIBridge: {
                    switch (static_cast<_PCIPCIBridgeProgIF>(ProgIF)) {
                        case _PCIPCIBridgeProgIF::NormalDecode:
                            return "Bridge, PCI to PCI, Normal Decode";
                        case _PCIPCIBridgeProgIF::SubtractiveDecode:
                            return "Bridge, PCI to PCI, Subtractive Decode";
                        default:
                            return "Bridge, PCI to PCI, Unknown";
                    }
                    break;
                }
                case BridgeSubClass::InfiniBandToPCIHostBridge:
                    return "Bridge, InfiniBand to PCI Host";
                    break;
                case BridgeSubClass::Other:
                    return "Bridge, Other";
                    break;
                default:
                    return "Bridge, Unknown";
            }
            break;
        }
        case ClassCodes::SimpleCommunicationController: {
            switch (static_cast<SimpleCommunicationControllerSubClass>(SubClass)) {
                case SimpleCommunicationControllerSubClass::SerialController: {
                    switch (static_cast<SerialControllerProgIF>(ProgIF)) {
                        case SerialControllerProgIF::8250Compatible:
                            return "Simple Communication Controller, Serial, 8250 Compatible";
                        case SerialControllerProgIF::16450Compatible:
                            return "Simple Communication Controller, Serial, 16450 Compatible";
                        case SerialControllerProgIF::16550Compatible:
                            return "Simple Communication Controller, Serial, 16550 Compatible";
                        case SerialControllerProgIF::16550ACompatible:
                            return "Simple Communication Controller, Serial, 16550A Compatible";
                        case SerialControllerProgIF::16650Compatible:
                            return "Simple Communication Controller, Serial, 16650 Compatible";
                        case SerialControllerProgIF::16750Compatible:
                            return "Simple Communication Controller, Serial, 16750 Compatible";
                        case SerialControllerProgIF::16850Compatible:
                            return "Simple Communication Controller, Serial, 16850 Compatible";
                        case SerialControllerProgIF::16950Compatible:
                            return "Simple Communication Controller, Serial, 16950 Compatible";
                        default:
                            return "Simple Communication Controller, Serial, Unknown";
                    }
                    break;
                }
                case SimpleCommunicationControllerSubClass::ParallelController: {
                    switch (static_cast<ParallelControllerProgIF>(ProgIF)) {
                        case ParallelControllerProgIF::StandardParallelPort:
                            return "Simple Communication Controller, Parallel, Standard";
                        case ParallelControllerProgIF::BiDirectionalParallelPort:
                            return "Simple Communication Controller, Parallel, Bi-Directional";
                        case ParallelControllerProgIF::ECPCompliantParallelPort:
                            return "Simple Communication Controller, Parallel, ECP Compliant";
                        case ParallelControllerProgIF::IEEE1284Controller:
                            return "Simple Communication Controller, Parallel, IEEE 1284 Controller";
                        case ParallelControllerProgIF::IEEE1284TargetDevice:
                            return "Simple Communication Controller, Parallel, IEEE 1284 Target Device";
                        default:
                            return "Simple Communication Controller, Parallel, Unknown";
                    }
                    break;
                }
                case SimpleCommunicationControllerSubClass::MultiportSerialController:
                    return "Simple Communication Controller, Multiport Serial";
                    break;
                case SimpleCommunicationControllerSubClass::Modem: {
                    switch (static_cast<ModemProgIF>(ProgIF)) {
                        case ModemProgIF::GenericModem:
                            return "Simple Communication Controller, Modem, Generic";
                        case ModemProgIF::Hayes16450Interface:
                            return "Simple Communication Controller, Modem, Hayes 16450 Interface";
                        case ModemProgIF::Hayes16550Interface:
                            return "Simple Communication Controller, Modem, Hayes 16550 Interface";
                        case ModemProgIF::Hayes16650Interface:
                            return "Simple Communication Controller, Modem, Hayes 16650 Interface";
                        case ModemProgIF::Hayes16750Interface:
                            return "Simple Communication Controller, Modem, Hayes 16750 Interface";
                        default:
                            return "Simple Communication Controller, Modem, Unknown";
                    }
                    break;
                }
                case SimpleCommunicationControllerSubClass::GPIBController:
                    return "Simple Communication Controller, GPIB";
                    break;
                case SimpleCommunicationControllerSubClass::SmartCardController:
                    return "Simple Communication Controller, Smart Card";
                    break;
                case SimpleCommunicationControllerSubClass::Other:
                    return "Simple Communication Controller, Other";
                    break;
                default:
                    return "Simple Communication Controller, Unknown";
            }
            break;
        }
        case ClassCodes::BaseSystemPeripheral: {
            switch (static_cast<BaseSystemPeripheralSubClass>(SubClass)) {
                case BaseSystemPeripheralSubClass::PIC: {
                    switch (static_cast<PICProgIF>(ProgIF)) {
                        case PICProgIF::Generic8259Compatible:
                            return "Base System Peripheral, PIC, Generic 8259 Compatible";
                        case PICProgIF::ISABusCompatible:
                            return "Base System Peripheral, PIC, ISA Bus Compatible";
                        case PICProgIF::EISABusCompatible:
                            return "Base System Peripheral, PIC, EISA Bus Compatible";
                        case PICProgIF::IOAPICInterruptController:
                            return "Base System Peripheral, PIC, IOAPIC Interrupt Controller";
                        case PICProgIF::IOxAPICInterruptController:
                            return "Base System Peripheral, PIC, IOxAPIC Interrupt Controller";
                        default:
                            return "Base System Peripheral, PIC, Unknown";
                    }
                    break;
                }
                case BaseSystemPeripheralSubClass::DMAController: {
                    switch (static_cast<DMAControllerProgIF>(ProgIF)) {
                        case DMAControllerProgIF::Generic8237Compatible:
                            return "Base System Peripheral, DMA, Generic 8237 Compatible";
                        case DMAControllerProgIF::ISABusCompatible:
                            return "Base System Peripheral, DMA, ISA Bus Compatible";
                        case DMAControllerProgIF::EISABusCompatible:
                            return "Base System Peripheral, DMA, EISA Bus Compatible";
                        default:
                            return "Base System Peripheral, DMA, Unknown";
                    }
                    break;
                }
                case BaseSystemPeripheralSubClass::Timer: {
                    switch (static_cast<TimerProgIF>(ProgIF)) {
                        case TimerProgIF::Generic8254Compatible:
                            return "Base System Peripheral, Timer, Generic 8254 Compatible";
                        case TimerProgIF::ISABusCompatible:
                            return "Base System Peripheral, Timer, ISA Bus Compatible";
                        case TimerProgIF::EISABusCompatible:
                            return "Base System Peripheral, Timer, EISA Bus Compatible";
                        case TimerProgIF::HPETimer:
                            return "Base System Peripheral, Timer, HPET";
                        default:
                            return "Base System Peripheral, Timer, Unknown";
                    }
                    break;
                }
                case BaseSystemPeripheralSubClass::RTCController: {
                    switch (static_cast<RTCControllerProgIF>(ProgIF)) {
                        case RTCControllerProgIF::GenericRTC:
                            return "Base System Peripheral, RTC, Generic";
                        case RTCControllerProgIF::ISABusCompatible:
                            return "Base System Peripheral, RTC, ISA Bus Compatible";
                        default:
                            return "Base System Peripheral, RTC, Unknown";
                    }
                    break;
                }
            }
            break;
        }
        case ClassCodes::InputDeviceController:
            return "Input Device Controller";
            break;
        case ClassCodes::DockingStation:
            return "Docking Station";
            break;
        case ClassCodes::Processor:
            return "Processor";
            break;
        case ClassCodes::SerialBusController:
            return "Serial Bus Controller";
            break;
        case ClassCodes::WirelessController:
            return "Wireless Controller";
            break;
        case ClassCodes::IntelligentController:
            return "Intelligent Controller";
            break;
        case ClassCodes::SatelliteCommunication:
            return "Satellite Communication Controller";
            break;
        case ClassCodes::EncryptionController:
            return "Encryption Controller";
            break;
        case ClassCodes::SignalProcessingController:
            return "Signal Processing Controller";
            break;
        case ClassCodes::ProcessingAccelerator:
            return "Processing Accelerator";
            break;
        default:
            return "Unknown Class Code";
            break;
    };
}

uint16_t PCI::getVendorID(uint8_t bus, uint8_t device, uint8_t function) {
    return ConfigReadWord(bus, device, function, 0x00);
}

uint8_t PCI::getHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    return ConfigReadWord(bus, device, function, 0x0E);
}

bool PCI::deviceAlreadyFound(uint8_t bus, uint8_t device, uint8_t function) {
    for (int i = 0; i < DeviceCount; i++) {
        if (Devices[i].bus == bus &&
            Devices[i].device == device &&
            Devices[i].function == function) {
            return true;
        }
    }
    return false;
}

void PCI::addDevice(uint8_t bus, uint8_t device, uint8_t function) {
    if (DeviceCount < MAX_DEVICES) {
        Devices[DeviceCount++] = {bus, device, function};
    }
}

/*
 * rn, this function just prints the device.
 * Later on, we can put this in a list
 * and use it to load drivers.
*/
void PCI::checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
    if (deviceAlreadyFound(bus, device, function)) return;

    addDevice(bus, device, function);

    uint16_t vendorID = getVendorID(bus, device, function);
    if (vendorID == 0xFFFF) return;

    uint8_t classCode = ConfigReadWord(bus, device, function, 0x0B);
    uint8_t subclass = ConfigReadWord(bus, device, function, 0x0A);
    uint8_t progIF = ConfigReadWord(bus, device, function, 0x09);

    ks->basicConsole.Println(GetDeviceCode(classCode, subclass, progIF));
}

/*
 * rn, this function just prints the device.
 * Later on, we can put this in a list
 * and use it to load drivers.
*/
void PCI::checkDevice(uint8_t bus, uint8_t device) {
    uint8_t function = 0;
    uint16_t vendorID = getVendorID(bus, device, function);
    if (vendorID == 0xFFFF) return;

    checkFunction(bus, device, function);

    uint8_t headerType = getHeaderType(bus, device, function);
    if ((headerType & 0x80) != 0) {
        for (function = 1; function < 8; function++) {
            if (getVendorID(bus, device, function) != 0xFFFF) {
                checkFunction(bus, device, function);
            }
        }
    }
}

/*
 * This is the Brute Force Method
 * to check all PCI devices.
 */
void PCI::checkAllBuses() {
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            checkDevice(bus, device);
        }
    }
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

bool PCI_Header::IOSpace() {
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
    return Status & (1 << 3);
}

/*
 * If set to 1 the device implements the pointer for a 
 * New Capabilities Linked list at offset 0x34; otherwise, 
 * the linked list is not available.
*/
bool PCI_Header::CapabilitiesList() {
    return Status & (1 << 4);
}

/*
 * If set to 1 the device is capable of running at 66 MHz; otherwise, the device runs at 33 MHz.
*/
bool PCI_Header::MHzCapable() {
    return Status & (1 << 5);
}

/*
 * If set to 1 the device can accept fast back-to-back 
 * transactions that are not from the same agent; otherwise, 
 * transactions can only be accepted from the same agent.
*/
bool PCI_Header::FastBBCapable() {
    return Status & (1 << 7);
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
    return Status & (1 << 8);
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
    return Status & (1 << 11);
}

/*
 * This bit will be set to 1, by a master device, whenever its 
 * transaction is terminated with Target-Abort.
*/
void PCI_Header::SetRecievedTargetAbort() {
    Status |= (1 << 12);
}

bool PCI_Header::RecievedTargetAbort() {
    return Status & (1 << 12);
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
    return Status & (1 << 13);
}

/*
 * This bit will be set to 1 whenever the device asserts SERR#.
*/
void PCI_Header::SetSignaledSystemError() {
    Status |= (1 << 14);
}

bool PCI_Header::SignaledSystemError() {
    return Status & (1 << 14);
}

/*
 * This bit will be set to 1 whenever the device detects a parity 
 * error, even if parity error handling is disabled.
*/
bool PCI_Header::DetectedParityError() {
    return Status & (1 << 15);
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