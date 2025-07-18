#include "PCIe.h"
#include "../KernelServices.h"

void PCIe::Initialize() {
    numSegments = (mcfgTable->Length - sizeof(MCFG)) / sizeof(MCFGEntry);
    checkAllSegments();
}

void PCIe::InitializePCIe(MCFG* mcfg) {
    mcfgTable = mcfg;
    Devices.clear();
}

/*
 * We can check if the PCIe exists
 * by checking if our MCFG Table
 * Exists or not. We can then verify
 * if our table is Indeed the MCFG
 * table by checking it signature.
*/
bool PCIe::PCIeExists() {
    if (mcfgTable == NULL) {
        return false;
    } else if (mcfgTable->Signature == "MCFG") {
        return true;
    }
    return false;
}

void PCIe::checkAllSegments() {
    for (size_t i = 0; i < numSegments; i++) {
        MCFGEntry entry = mcfgTable->entries[i];
        checkBus(entry.PCISegmentGroupNum, 0);
    }
}

/*
 * This is the Recursive Method
 * to check all PCI devices.
 */
void PCIe::checkBus(uint16_t segment, uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        checkDevice(segment, bus, device);
    }
}

/*
 * We can use this func to check the
 * device in a specific segment and
 * add it to our devices array.
*/
void PCIe::checkDevice(uint16_t segment, uint8_t bus, uint8_t device) {
    uint8_t function = 0;
    uint16_t vendorID = getVendorID(segment, bus, device, function);
    if (vendorID == 0xFFFF) return;

    checkFunction(segment, bus, device, function);

    uint8_t headerType = getHeaderType(segment, bus, device, function);
    if ((headerType & 0x80) != 0) {
        for (function = 1; function < 8; function++) {
            if (getVendorID(segment, bus, device, function) != 0xFFFF) {
                checkFunction(segment, bus, device, function);
            }
        }
    }
}

/*
 * rn, this function just prints the device.
 * Later on, we can put this in a list
 * and use it to load drivers.
 * 
 * OK, it now add's the device. So now, we can
 * scan PCI to PCI Bridges to get all devices 
 * behind it.
 * 
 * tbf, I just copied this from the PCI class,
 * since both are really similar.
*/
void PCIe::checkFunction(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function) {
    if (deviceAlreadyFound(segment, bus, device, function)) return;

    bool hasMSIx = false;

    if ((ConfigReadWord(segment, bus, device, function, 0x06) >> 4) & 1) {
        uint8_t capPtr = ConfigReadByte(segment, bus, device, function, 0x34);
        while (capPtr != 0) {
            uint8_t capID = ConfigReadByte(segment, bus, device, function, capPtr);
            if (capID == 0x11) {
                hasMSIx = true;
                break;
            }
            capPtr = ConfigReadByte(segment, bus, device, function, capPtr + 1);
        }
    }

    uint16_t vendorID = getVendorID(segment, bus, device, function);
    if (vendorID == 0xFFFF) return;

    uint8_t classCode = ConfigReadWord(segment, bus, device, function, 0x0B);
    uint8_t subclass = ConfigReadWord(segment, bus, device, function, 0x0A);
    uint8_t progIF = ConfigReadWord(segment, bus, device, function, 0x09);

    addDevice(segment, bus, device, function, hasMSIx, vendorID, classCode, subclass, progIF);

    ks->basicConsole.Println(GetDeviceCode(classCode, subclass, progIF));

    if (classCode == 0x06 && subclass == 0x04) {
        uint8_t secondaryBus = ConfigReadWord(segment, bus, device, function, 0x19) & 0xFF;
        checkBus(segment, secondaryBus);
    }
}

/*
 * We can use this to quickly get the device class
*/
uint32_t PCIe::GetDeviceClass(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF) {
    return (static_cast<uint32_t>(ClassCode) << 24) |
           (static_cast<uint32_t>(SubClass)  << 16) |
           (static_cast<uint32_t>(ProgIF)    << 8);
}

/*
 * We can use this to get the class code
 * as a string.
*/
char* PCIe::GetClassCode(uint8_t ClassCode) {
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

char* PCIe::GetDeviceCode(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF) {
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
                case BridgeSubClass::PCIToPCIBridge: {
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
                        case RACEwayBridgeProgIF::EndpointMode:
                            return "Bridge, RACEway, Endpoint Mode";
                        default:
                            return "Bridge, RACEway, Unknown";
                    }
                    break;
                }
                case BridgeSubClass::_PCIToPCIBridge: {
                    switch (static_cast<_PCIPCIBridgeProgIF>(ProgIF)) {
                        case _PCIPCIBridgeProgIF::PrimaryBus:
                            return "Bridge, PCI to PCI, Semi-Transparant - Primary Bus";
                            break;
                        case _PCIPCIBridgeProgIF::SecondaryBus:
                            return "Bridge, PCI to PCI, Semi-Transparant - Secondary Bus";
                            break;
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
                        case SerialControllerProgIF::_8250:
                            return "Simple Communication Controller, Serial, 8250 Compatible";
                            break;
                        case SerialControllerProgIF::_16450:
                            return "Simple Communication Controller, Serial, 16450 Compatible";
                            break;
                        case SerialControllerProgIF::_16550:
                            return "Simple Communication Controller, Serial, 16550 Compatible";
                            break;
                        case SerialControllerProgIF::_16650:
                            return "Simple Communication Controller, Serial, 16650 Compatible";
                            break;
                        case SerialControllerProgIF::_16750:
                            return "Simple Communication Controller, Serial, 16750 Compatible";
                            break;
                        case SerialControllerProgIF::_16850:
                            return "Simple Communication Controller, Serial, 16850 Compatible";
                            break;
                        case SerialControllerProgIF::_16950:
                            return "Simple Communication Controller, Serial, 16950 Compatible";
                            break;
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
                        case PICProgIF::ISACompatible:
                            return "Base System Peripheral, PIC, ISA Bus Compatible";
                        case PICProgIF::EISACompatible:
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
                        case DMAControllerProgIF::ISACompatible:
                            return "Base System Peripheral, DMA, ISA Bus Compatible";
                        case DMAControllerProgIF::EISACompatible:
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
                        case TimerProgIF::ISACompatible:
                            return "Base System Peripheral, Timer, ISA Bus Compatible";
                        case TimerProgIF::EISACompatible:
                            return "Base System Peripheral, Timer, EISA Bus Compatible";
                        case TimerProgIF::HPET:
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
                        case RTCControllerProgIF::ISACompatible:
                            return "Base System Peripheral, RTC, ISA Bus Compatible";
                        default:
                            return "Base System Peripheral, RTC, Unknown";
                    }
                    break;
                }
                case BaseSystemPeripheralSubClass::PCIHotPlugController:
                    return "Base System Peripheral, PCI Hot Plug Controller";
                    break;
                case BaseSystemPeripheralSubClass::SDHostController:
                    return "Base System Peripheral, SD Host Controller";
                    break;
                case BaseSystemPeripheralSubClass::IOMMU:
                    return "Base System Peripheral, IOMMU";
                    break;
                case BaseSystemPeripheralSubClass::Other:
                    return "Base System Peripheral, Other";
                    break;
                default:
                    return "Base System Peripheral, Unknown";
            }
            break;
        }
        case ClassCodes::InputDeviceController: {
            switch (static_cast<InputDeviceControllerSubClass>(SubClass)) {
                case InputDeviceControllerSubClass::KeyboardController:
                    return "Input Device Controller, Keyboard";
                    break;
                case InputDeviceControllerSubClass::DigitizerPen:
                    return "Input Device Controller, Digitizer Pen";
                    break;
                case InputDeviceControllerSubClass::MouseController: {
                    return "Input Device Controller, Mouse Controller";
                    break;
                }
                case InputDeviceControllerSubClass::ScannerController:
                    return "Input Device Controller, Scanner";
                    break;
                case InputDeviceControllerSubClass::GameportController: {
                    switch (static_cast<GameportControllerProgIF>(ProgIF)) {
                        case GameportControllerProgIF::Generic:
                            return "Input Device Controller, Gameport, Generic";
                        case GameportControllerProgIF::Extended:
                            return "Input Device Controller, Gameport, Extended";
                        default:
                            return "Input Device Controller, Gameport, Unknown";
                    }
                    break;
                }
                case InputDeviceControllerSubClass::Other:
                    return "Input Device Controller, Other";
                    break;
                default:
                    return "Input Device Controller, Unknown";
            }
            break;
        }
        case ClassCodes::DockingStation: {
            switch (static_cast<DockingStationSubClass>(SubClass)) {
                case DockingStationSubClass::Generic:
                    return "Docking Station, Generic";
                    break;
                case DockingStationSubClass::Other:
                    return "Docking Station, Other";
                    break;
                default:
                    return "Docking Station, Unknown";
            }
            break;
        }
        case ClassCodes::Processor: {
            switch (static_cast<ProcessorSubClass>(SubClass)) {
                case ProcessorSubClass::_386:
                    return "Processor, 386";
                    break;
                case ProcessorSubClass::_486:
                    return "Processor, 486";
                    break;
                case ProcessorSubClass::Pentium:
                    return "Processor, Pentium";
                    break;
                case ProcessorSubClass::PentiumPro:
                    return "Processor, Pentium Pro";
                    break;
                case ProcessorSubClass::Alpha:
                    return "Processor, Alpha";
                    break;
                case ProcessorSubClass::PowerPC:
                    return "Processor, PowerPC";
                    break;
                case ProcessorSubClass::MIPS:
                    return "Processor, MIPS";
                    break;
                case ProcessorSubClass::CoProcessor:
                    return "Processor, Co-Processor";
                    break;
                case ProcessorSubClass::Other:
                    return "Processor, Other";
                    break;
                default:
                    return "Processor, Unknown";
            }
            break;
        }
        case ClassCodes::SerialBusController: {
            switch (static_cast<SerialBusControllerSubClass>(SubClass)) {
                case SerialBusControllerSubClass::FireWireController: {
                    switch (static_cast<FirewireControllerProgIF>(ProgIF)) {
                        case FirewireControllerProgIF::Generic:
                            return "Serial Bus Controller, FireWire, Generic"; 
                            break;
                        case FirewireControllerProgIF::OCHI:
                            return "Serial Bus Controller, FireWire, OHCI";
                            break;
                        default:
                            return "Serial Bus Controller, FireWire, Unknown";
                    }
                    break;
                }
                case SerialBusControllerSubClass::AccessBusController:
                    return "Serial Bus Controller, Access Bus";
                    break;
                case SerialBusControllerSubClass::SSA:
                    return "Serial Bus Controller, SSA";
                    break;
                case SerialBusControllerSubClass::USBController: {
                    switch (static_cast<USBControllerProgIF>(ProgIF)) {
                        case USBControllerProgIF::UHCI:
                            return "Serial Bus Controller, USB, UHCI";
                        case USBControllerProgIF::OHCI:
                            return "Serial Bus Controller, USB, OHCI";
                        case USBControllerProgIF::EHCI:
                            return "Serial Bus Controller, USB, EHCI";
                        case USBControllerProgIF::XHCI:
                            return "Serial Bus Controller, USB, XHCI";
                        case USBControllerProgIF::Unspecified:
                            return "Serial Bus Controller, USB, Unspecified";
                        case USBControllerProgIF::USBDevice:
                            return "Serial Bus Controller, USB, USB Device";
                        default:
                            return "Serial Bus Controller, USB, Unknown";
                    }
                    break;
                }
                case SerialBusControllerSubClass::FibreChannel:
                    return "Serial Bus Controller, Fibre Channel";
                    break;
                case SerialBusControllerSubClass::SMBusController:
                    return "Serial Bus Controller, SMBus";
                    break;
                case SerialBusControllerSubClass::InfiniBandController:
                    return "Serial Bus Controller, InfiniBand";
                    break;
                case SerialBusControllerSubClass::IPMIInterface: {
                    switch (static_cast<IPMIInterfaceProgIF>(ProgIF)) {
                        case IPMIInterfaceProgIF::SMIC:
                            return "Serial Bus Controller, IPMI Interface, SMIC";
                        case IPMIInterfaceProgIF::KeyboardControllerStyle:
                            return "Serial Bus Controller, IPMI Interface, Keyboard Controller Style";
                        case IPMIInterfaceProgIF::BlockTransfer:
                            return "Serial Bus Controller, IPMI Interface, Block Transfer";
                        default:
                            return "Serial Bus Controller, IPMI Interface, Unknown";
                    }
                    break;
                }
                case SerialBusControllerSubClass::SERCOSInterface:
                    return "Serial Bus Controller, SERCOS Interface";
                    break;
                case SerialBusControllerSubClass::CANbusController:
                    return "Serial Bus Controller, CANbus";
                    break;
                case SerialBusControllerSubClass::Other:
                    return "Serial Bus Controller, Other";
                    break;
                default:
                    return "Serial Bus Controller, Unknown";
            }
        }
        case ClassCodes::WirelessController: {
            switch (static_cast<WirelessControllerSubClass>(SubClass)) {
                case WirelessControllerSubClass::IRDACompatibleController:
                    return "Wireless Controller, IRDA Compatible";
                    break;
                case WirelessControllerSubClass::ConsumerIRController:
                    return "Wireless Controller, Consumer IR";
                    break;
                case WirelessControllerSubClass::RFController:
                    return "Wireless Controller, RF";
                    break;
                case WirelessControllerSubClass::BluetoothController:
                    return "Wireless Controller, Bluetooth";
                    break;
                case WirelessControllerSubClass::BroadbandController:
                    return "Wireless Controller, Broadband";
                    break;
                case WirelessControllerSubClass::EthernetController8021a:
                    return "Wireless Controller, Ethernet 802.1a";
                    break;
                case WirelessControllerSubClass::EthernetController8021b:
                    return "Wireless Controller, Ethernet 802.1b";
                    break;
                case WirelessControllerSubClass::Other:
                    return "Wireless Controller, Other";
                    break;
                default:
                    return "Wireless Controller, Unknown";
            }
        }
        case ClassCodes::IntelligentController: {
            switch (static_cast<IntelligentControllerSubClass>(SubClass)) {
                case IntelligentControllerSubClass::l20:
                    return "Intelligent Controller, l20";
                    break;
                default:
                    return "Intelligent Controller, Unknown";
            }
            break;
        }
        case ClassCodes::SatelliteCommunication: {
            switch (static_cast<SatelliteCommunicationSubClass>(SubClass)) {
                case SatelliteCommunicationSubClass::SatelliteTVController:
                    return "Satellite Communication, Satellite TV";
                    break;
                case SatelliteCommunicationSubClass::SatelliteAudioController:
                    return "Satellite Communication, Satellite Audio";
                    break;
                case SatelliteCommunicationSubClass::SatelliteVoiceController:
                    return "Satellite Communication, Satellite Voice";
                    break;
                case SatelliteCommunicationSubClass::SatelliteDataController:
                    return "Satellite Communication, Satellite Data";
                    break;
                default:
                    return "Satellite Communication, Unknown";
            }
            break;
        }
        case ClassCodes::EncryptionController: {
            switch (static_cast<EncryptionControllerSubClass>(SubClass)) {
                case EncryptionControllerSubClass::NetworkComputingEnDecryption:
                    return "Encryption Controller, Network Computing Encryption/Decryption";
                    break;
                case EncryptionControllerSubClass::EntertainmentEnDecryption:
                    return "Encryption Controller, Entertainment Encryption/Decryption";
                    break;
                case EncryptionControllerSubClass::Other:
                    return "Encryption Controller, Other";
                    break;
                default:
                    return "Encryption Controller, Unknown";
            }
            break;
        }
        case ClassCodes::SignalProcessingController: {
            switch (static_cast<SignalProcessingControllerSubClass>(SubClass)) {
                case SignalProcessingControllerSubClass::DPIOModules:
                    return "Signal Processing Controller, DPIO Modules";
                    break;
                case SignalProcessingControllerSubClass::PerformanceCounters:
                    return "Signal Processing Controller, Performance Counters";
                    break;
                case SignalProcessingControllerSubClass::CommunicationSynchronizer:
                    return "Signal Processing Controller, Communication Synchronizer";
                    break;
                case SignalProcessingControllerSubClass::SignalProcessingManagement:
                    return "Signal Processing Controller, Signal Processing Management";
                    break;
                case SignalProcessingControllerSubClass::Other:
                    return "Signal Processing Controller, Other";
                    break;
                default:
                    return "Signal Processing Controller, Unknown";
            }
            break;
        }
        case ClassCodes::ProcessingAccelerator:
            return "Processing Accelerator";
            break;
        case ClassCodes::NonEssentialInstrumentation:
            return "Non-Essential Instrumentation";
            break;
        case ClassCodes::x3FReserved:
            return "0x3F Reserved";
            break;
        case ClassCodes::CoProcessor:
            return "Co-Processor";
            break;
        case ClassCodes::xFEReserved:
            return "0xFE Reserved";
            break;
        case ClassCodes::UnAssignedClass:
            return "Unassigned Class";
            break;
        default:
            return "Unknown Class Code";
            break;
    };
}

volatile uint32_t* PCIe::GetECAMBase(uint16_t segment) {
    for (int i = 0; i < numSegments; i++) {
        if (mcfgTable->entries[i].PCISegmentGroupNum == segment) {
            return (volatile uint32_t*)(uintptr_t)(mcfgTable->entries[i].BaseAddr);
        }
    }
    return nullptr;
}

uint16_t PCIe::ConfigReadWord(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t dword = ConfigReadDWord(segment, bus, slot, func, offset & ~0x3);
    uint8_t shift = (offset & 2) * 8;
    return (dword >> shift) & 0xFFFF;
}

void PCIe::ConfigWriteWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value) {
    volatile uint32_t* ecam_base = GetECAMBase(segment);
    if (!ecam_base) return;

    uint64_t ecam_offset = ((uint64_t)bus << 20) 
                         | ((uint64_t)device << 15) 
                         | ((uint64_t)function << 12) 
                         | (offset & ~0x3);

    volatile uint32_t* addr = (volatile uint32_t*)((uintptr_t)ecam_base + ecam_offset);

    uint32_t current_value = *addr;
    uint32_t shift = (offset & 2) * 8;
    uint32_t mask = 0xFFFF << shift;

    uint32_t new_value = (current_value & ~mask) | ((uint32_t)value << shift);
    *addr = new_value;
}

void PCIe::ConfigWriteDWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    volatile uint32_t* ecam_base = GetECAMBase(segment);
    if (!ecam_base) return;

    uint64_t ecam_offset = ((uint64_t)bus << 20) 
                         | ((uint64_t)device << 15) 
                         | ((uint64_t)function << 12)
                         | (offset & ~0x3);

    volatile uint32_t* addr = (volatile uint32_t*)((uintptr_t)ecam_base + ecam_offset);
    *addr = value;
}

uint8_t PCIe::ConfigReadByte(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t dword = ConfigReadDWord(segment, bus, device, function, offset & ~0x3);
    uint8_t shift = (offset & 3) * 8;
    return (dword >> shift) & 0xFF;
}

uint32_t PCIe::ConfigReadDWord(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    volatile uint32_t* ecam_base = GetECAMBase(segment);
    if (!ecam_base) return 0xFFFFFFFF;
    
    uint64_t ecam_offset = ((uint64_t)bus << 20)
                         | ((uint64_t)device << 15)
                         | ((uint64_t)function << 12)
                         | (offset & ~0x3);

    volatile uint32_t* addr = (volatile uint32_t*)((uintptr_t)ecam_base + ecam_offset);
    return *addr;
}

uint16_t PCIe::getVendorID(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function) {
    return ConfigReadWord(segment, bus, device, function, 0x00);
}

uint8_t PCIe::getHeaderType(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function) {
    return ConfigReadWord(segment, bus, device, function, 0x0E);
}

bool PCIe::deviceAlreadyFound(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function) {
    for (int i = 0; i < Devices.size; i++) {
        if (Devices[i].segment == segment &&
            Devices[i].bus == bus &&
            Devices[i].device == device &&
            Devices[i].function == function) {
            return true;
        }
    }
    return false;
}

void PCIe::addDevice(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, bool hasMSIx, uint16_t vendorID, uint8_t classCode, uint8_t subClass, uint8_t progIF) {
    DeviceKey dev;
    dev.segment = segment;
    dev.bus = bus;
    dev.device = device;
    dev.function = function;
    dev.hasMSIx = hasMSIx;
    dev.hasMSI = false;
    dev.PCIe = true;

    dev.vendorID = vendorID;

    dev.classCode = classCode;
    dev.subclass = subClass;
    dev.progIF = progIF;

    for (int i = 0; i < 6; i++) {
        dev.bars[i] = ConfigReadDWord(segment, bus, device, function, 0x10 + (i * 4));
    }

    Devices.push_back(dev);
}

/*
 * This too, was made by ChatGPT.
*/
Array<DeviceKey> PCIe::GetDevices() {
    return Devices;
}

bool PCIe::EnableMSIx(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint8_t vector) {
    ks->basicConsole.Println("MSI-X isn't implemented yet!");
}