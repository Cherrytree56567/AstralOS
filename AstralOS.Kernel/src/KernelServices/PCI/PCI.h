#pragma once
#include <tuple>
#include "../CPUutils/cpuid.h"
#include "../Array/Array.h"

/*
 * This code is from the OSDev Wiki:
 * https://wiki.osdev.org/PCI#The_PCI_Bus
*/
enum class PCIHeaderType : uint8_t {
    General = 0x0,
    PCI_PCI_BRIDGE = 0x1,
    PCI_CARDBus_BRIDGE = 0x2
};

enum class DEVSEL : uint8_t {
    Fast = 0,
    Medium = 1,
    Slow = 2,
    Reserved = 3
};

struct MemSpaceBAR {
    uint32_t Identifier : 1;    // Always 0
    uint32_t Type : 2;
    uint32_t Prefetchable : 1;
    uint32_t BaseAddr : 28; // 16-Byte Aligned Base Address
} __attribute__((packed));

struct IOSpaceBAR {
    uint32_t Identifier : 1;    // Always 1
    uint32_t Reserved : 1;
    uint32_t BaseAddr : 30; // 4-Byte Aligned Base Address 
} __attribute__((packed));

struct GenericBAR {
    uint32_t Identifier : 1;
    uint32_t Reserved : 31;
} __attribute__((packed));

static_assert(sizeof(MemSpaceBAR) == 4, "BAR struct must be 32 bits");
static_assert(sizeof(IOSpaceBAR) == 4, "BAR struct must be 32 bits");
static_assert(sizeof(GenericBAR) == 4, "BAR struct must be 32 bits");

struct PCI_Header {
    uint16_t VendorID;      // Identifies the manufacturer
    uint16_t DeviceID;      // Identifies the Device
    uint16_t Command;       // Provides control over a device's ability to generate and respond to PCI cycles.
    uint16_t Status;        // A register used to record status information for PCI bus related events.
    uint8_t RevisionID;     // Specifies a revision identifier for a particular device.
    uint8_t ProgIF;         // Programming Interface Byte.
    uint8_t Subclass;       // A read-only register that specifies the specific function the device performs.
    uint8_t ClassCode;      // A read-only register that specifies the type of function the device performs.
    uint8_t CacheLineSize;  // Specifies the system cache line size in 32-bit units.
    uint8_t LatencyTimer;   // Specifies the latency timer in units of PCI bus clocks.
    uint8_t HeaderType;     // Identifies the layout of the rest of the header beginning at byte 0x10 of the header.
    uint8_t BIST;           // Represents that status and allows control of a devices BIST.

    /*
     * Command Register
    */
    void SetIOSpace(bool val);
    bool IOSpace();
    void SetMemorySpace(bool val);
    bool MemorySpace();
    void SetBusMaster(bool val);
    bool BusMaster();
    bool SpecialCycles();
    bool MemWrite();
    bool VGAPaletteSnoop();
    void SetParityErrorResponse(bool val);
    bool ParityErrorResponse();
    void SetSERREnable(bool val);
    bool SERREnable();
    bool FastBBEnable();
    void SetInterruptDisable(bool val);
    bool InterruptDisable();

    /*
     * Status Register
    */
    bool InterruptStatus();
    bool CapabilitiesList();
    bool MHzCapable();
    bool FastBBCapable();
    bool MasterDataParityError();
    DEVSEL DEVSELTiming();
    bool SignaledTargetAbort();
    bool RecievedTargetAbort();
    bool RecievedMasterAbort();
    bool SignaledSystemError();
    bool DetectedParityError();
    void SetMasterDataParityError();
    void SetSignaledTargetAbort();
    void SetRecievedTargetAbort();
    void SetRecievedMasterAbort();
    void SetSignaledSystemError();
    void SetDetectedParityError();

    PCIHeaderType GetHeaderType();
    bool IsMultiFunction();

    /*
     * BIST
    */
    uint8_t GetCompletionCode();
    bool IsStartBIST();
    void SetStartBIST(bool start);
    bool IsBISTCapable();
} __attribute__((packed));

struct GeneralPCI_Header {
    PCI_Header pci;
    GenericBAR BAR0;                // Base address #0 (BAR0) 
    GenericBAR BAR1;                // Base address #1 (BAR1) 
    GenericBAR BAR2;                // Base address #2 (BAR2) 
    GenericBAR BAR3;                // Base address #3 (BAR3) 
    GenericBAR BAR4;                // Base address #4 (BAR4) 
    GenericBAR BAR5;                // Base address #5 (BAR5)
    uint32_t CardbusCISPointer;     // Points to the Card Information Structure and is used by devices that share silicon between CardBus and PCI.
    uint16_t SubsystemVendorID;
    uint16_t SubsystemID;
    uint32_t ExpansionROMBaseAddr;
    uint8_t CapabilitiesPointer;
    uint8_t Reserved0;
    uint16_t Reserved1;
    uint32_t Reserved2;
    uint8_t InterruptLine;
    uint8_t InterruptPin;
    uint8_t MinGrant;
    uint8_t MaxLatency;
} __attribute__((packed));

struct PCIBRIDGE_Header {
    PCI_Header pci;
    GenericBAR BAR0;
    GenericBAR BAR1;
    uint8_t PrimaryBusNum;
    uint8_t SecondaryBusNum;
    uint8_t SubordinateBusNum;
    uint8_t SecondaryLatencyTimer;
    uint8_t IOBase;
    uint8_t IOLimit;
    uint16_t SecondaryStatus;
    uint16_t MemoryBase;
    uint16_t MemoryLimit;
    uint16_t PrefetchableMemoryBase;
    uint16_t PrefetchableMemoryLimit;
    uint32_t PrefetchableBaseUpper32;
    uint32_t PrefetchableLimitUpper32;
    uint16_t IOBaseUpper16;
    uint16_t IOLimitUpper16;
    uint8_t CapabilityPointer;
    uint8_t Reserved0;
    uint16_t Reserved1;
    uint32_t ExpansionROMBaseAddr;
    uint8_t InterruptLine;
    uint8_t InterruptPin;
    uint16_t BridgeControl;
} __attribute__((packed));

struct PCICardBus_Header {
    PCI_Header pci;
    uint32_t CardBusBaseAddr;
    uint8_t OffsetCapabilitiesList;
    uint8_t Reserved;
    uint16_t SecondaryStatus;
    uint8_t PCIBusNum;
    uint8_t CardBusNum;
    uint8_t SubordinateBusNum;
    uint8_t CardBusLatencyTimer;
    MemSpaceBAR MemoryBaseAddr0;
    uint32_t MemoryLimit0;
    MemSpaceBAR MemoryBaseAddr1;
    uint32_t MemoryLimit1;
    IOSpaceBAR IOBaseAddr0;
    uint32_t IOLimit0;
    IOSpaceBAR IOBaseAddr1;
    uint32_t IOLimit1;
    uint8_t InterruptLine;
    uint8_t InterruptPin;
    uint16_t BridgeControl;
    uint16_t SubsystemDeviceID;
    uint16_t SubsystemVendorID;
    uint32_t _16BitPCCardLegacyModeBaseAddr;
} __attribute__((packed));

enum class ClassCodes {
    Unclassified = 0x0,
    MassStorageController = 0x1,
    NetworkController = 0x2,
    DisplayController = 0x3,
    MultimediaController = 0x4,
    MemoryController = 0x5,
    Bridge = 0x6,
    SimpleCommunicationController = 0x7,
    BaseSystemPeripheral = 0x8,
    InputDeviceController = 0x9,
    DockingStation = 0xA,
    Processor = 0xB,
    SerialBusController = 0xC,
    WirelessController = 0xD,
    IntelligentController = 0xE,
    SatelliteCommunication = 0xF,
    EncryptionController = 0x10,
    SignalProcessingController = 0x11,
    ProcessingAccelerator = 0x12,
    NonEssentialInstrumentation = 0x13,
    x3FReserved = 0x14,
    CoProcessor = 0x40,
    xFEReserved = 0x41,
    UnAssignedClass = 0xFF // Vendor Specific
};

enum UnclassifiedSubClass {
    Non_VGACompatible = 0x0,
    VGACompatible = 0x1
};

enum class MassStorageControllerSubClass {
    SCSIBusController = 0x0,
    IDEController = 0x01,
    FloppyDiskController = 0x02,
    IPIBusController = 0x03,
    RAIDController = 0x04,
    ATAController = 0x05,
    SerialATAController = 0x06,
    SerialAttachedSCSIController = 0x07,
    Non_VolatileMemoryController = 0x08,
    Other = 0x80
};

enum class NetworkControllerSubClass {
    EthernetController = 0x0,
    TokenRingController = 0x1,
    FDDIController = 0x2,
    ATMController = 0x3,
    ISDNController = 0x4,
    WorldFipController = 0x5,
    PICMGController = 0x6,
    InfinibandController = 0x7,
    FabricController = 0x8,
    Other = 0x80
};

enum class DisplayControllerSubClass {
    VGACompatibleController = 0x0,
    XGAController = 0x1,
    _3DController = 0x2,
    Other = 0x80
};

enum class MultimediaControllerSubClass {
    MultimediaVideoController = 0x0,
    MultimediaAudioController = 0x1,
    ComputerTelephonyDevice = 0x2,
    AudioDevice = 0x3,
    Other = 0x80
};

enum class MemoryControllerSubClass {
    RAMController = 0x0,
    FlashController = 0x1,
    Other = 0x80
};

enum class BridgeSubClass {
    HostBridge = 0x0,
    ISABridge = 0x1,
    EISABridge = 0x2,
    MCABridge = 0x3,
    PCIToPCIBridge = 0x4,
    PCMCIABridge = 0x5,
    NuBusBridge = 0x6,
    CardBusBridge = 0x7,
    RACEwayBridge = 0x8,
    _PCIToPCIBridge = 0x9,
    InfiniBandToPCIHostBridge = 0x0A,
    Other = 0x80
};

enum class SimpleCommunicationControllerSubClass {
    SerialController = 0x0,
    ParallelController = 0x1,
    MultiportSerialController = 0x2,
    Modem = 0x3,
    GPIBController = 0x4,
    SmartCardController = 0x5,
    Other = 0x80
};

enum class BaseSystemPeripheralSubClass {
    PIC = 0x0,
    DMAController = 0x01,
    Timer = 0x02,
    RTCController = 0x3,
    PCIHotPlugController = 0x4,
    SDHostController = 0x5,
    IOMMU = 0x6,
    Other = 0x80
};

enum class InputDeviceControllerSubClass {
    KeyboardController = 0x0,
    DigitizerPen = 0x1,
    MouseController = 0x2,
    ScannerController = 0x3,
    GameportController = 0x4,
    Other = 0x80
};

enum class DockingStationSubClass {
    Generic = 0x0,
    Other = 0x80
};

enum class ProcessorSubClass {
    _386 = 0x0,
    _486 = 0x1,
    Pentium = 0x2,
    PentiumPro = 0x3,
    Alpha = 0x10,
    PowerPC = 0x20,
    MIPS = 0x30,
    CoProcessor = 0x40,
    Other = 0x80
};

enum class SerialBusControllerSubClass {
    FireWireController = 0x0,
    AccessBusController = 0x1,
    SSA = 0x2,
    USBController = 0x3,
    FibreChannel = 0x4,
    SMBusController = 0x5,
    InfiniBandController = 0x6,
    IPMIInterface = 0x7,
    SERCOSInterface = 0x8,
    CANbusController = 0x9,
    Other = 0x80
};

enum class WirelessControllerSubClass {
    IRDACompatibleController = 0x0,
    ConsumerIRController = 0x1,
    RFController = 0x10,
    BluetoothController = 0x11,
    BroadbandController = 0x12,
    EthernetController8021a = 0x20,
    EthernetController8021b = 0x21,
    Other = 0x80
};

enum class IntelligentControllerSubClass {
    l20 = 0x0
};

enum class SatelliteCommunicationSubClass {
    SatelliteTVController = 0x1,
    SatelliteAudioController = 0x2,
    SatelliteVoiceController = 0x3,
    SatelliteDataController = 0x4
};

enum class EncryptionControllerSubClass {
    NetworkComputingEnDecryption = 0x0,
    EntertainmentEnDecryption = 0x10,
    Other = 0x80
};

enum class SignalProcessingControllerSubClass {
    DPIOModules = 0x0,
    PerformanceCounters = 0x1,
    CommunicationSynchronizer = 0x10,
    SignalProcessingManagement = 0x20,
    Other = 0x80
};

enum class IDEControllerProgIF {
    ISACompatibilityModeOnlyController = 0x0,
    PCINativeModeOnlyController = 0x5,
    ISACompatibilityModeController = 0xA,
    PCINativeModeController = 0xF,
    ISACompatibilityModeOnlyControllerBusMastering = 0x80,
    PCINativeModeOnlyControllerBusMastering = 0x85,
    ISACompatibilityModeControllerBusMastering = 0x8A,
    PCINativeModeControllerBusMastering = 0x8F
};

enum class ATAControllerProgIF {
    SingleDMA = 0x20,
    ChainedDMA = 0x30
};

enum class SATAControllerProgIF {
    VendorSpecificInterface = 0x0,
    ACHI1_0 = 0x1,
    SerialStorageBus = 0x2
};

enum class SerialAttachedSCSIControllerProgIF {
    SAS = 0x0,
    SerialStorageBus = 0x1
};

enum class NonVolatileMemoryControllerProgIF {
    NVMHCI = 0x1,
    NVMExpress = 0x2
};

enum class VGACompatibleControllerProgIF {
    VGAController = 0x0,
    _8514CompatibleController = 0x1
};

enum class PCIPCIBridgeProgIF {
    NormalDecode = 0x0,
    SubtractiveDecode = 0x1
};

enum class RACEwayBridgeProgIF {
    TransparantMode = 0x0,
    EndpointMode = 0x1
};

enum class _PCIPCIBridgeProgIF {
    PrimaryBus = 0x40,
    SecondaryBus = 0x80
};

enum class SerialControllerProgIF {
    _8250 = 0x0,
    _16450 = 0x1,
    _16550 = 0x2,
    _16650 = 0x3,
    _16750 = 0x4,
    _16850 = 0x5,
    _16950 = 0x6
};

enum class ParallelControllerProgIF {
    StandardParallelPort = 0x0,
    BiDirectionalParallelPort = 0x1,
    ECPCompliantParallelPort = 0x2,
    IEEE1284Controller = 0x3,
    IEEE1284TargetDevice = 0xFE
};

enum class ModemProgIF {
    GenericModem = 0x0,
    Hayes16450Interface = 0x1,
    Hayes16550Interface = 0x2,
    Hayes16650Interface = 0x3,
    Hayes16750Interface = 0x4
};

enum class PICProgIF {
    Generic8259Compatible = 0x0,
    ISACompatible = 0x1,
    EISACompatible = 0x2,
    IOAPICInterruptController = 0x10,
    IOxAPICInterruptController = 0x20
};

enum class DMAControllerProgIF {
    Generic8237Compatible = 0x0,
    ISACompatible = 0x1,
    EISACompatible = 0x2
};

enum class TimerProgIF {
    Generic8254Compatible = 0x0,
    ISACompatible = 0x1,
    EISACompatible = 0x2,
    HPET = 0x3
};

enum class RTCControllerProgIF {
    GenericRTC = 0x0,
    ISACompatible = 0x1
};

enum class GameportControllerProgIF {
    Generic = 0x0,
    Extended = 0x10
};

enum class FirewireControllerProgIF {
    Generic = 0x0,
    OCHI = 0x10
};

enum class USBControllerProgIF {
    UHCI = 0x0,
    OHCI = 0x10,
    EHCI = 0x20,    // USB2
    XHCI = 0x30,    // USB3
    Unspecified = 0x80,
    USBDevice = 0xFE
};

enum class IPMIInterfaceProgIF {
    SMIC = 0x0,
    KeyboardControllerStyle = 0x1,
    BlockTransfer = 0x2
};

struct MSIMCRegister {
    uint16_t Enable : 1;
    uint16_t MultipleMessageCapable : 3;
    uint16_t MultipleMessageEnable : 3;
    uint16_t _64Bit : 1;
    uint16_t PerVectorMasking : 1;
    uint16_t Reserved : 7;
} __attribute__((packed));

struct MSICapability {
    uint8_t CapabilityID;
    uint8_t NextPointer;
    MSIMCRegister MessageControl;
    uint32_t MessageAddrLow;
    uint32_t MessageAddrHigh;
    uint16_t MessageData;
    uint16_t Reserved;
    uint32_t Mask;
    uint32_t Pending;
} __attribute__((packed));

struct DeviceKey {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    bool hasMSI;
};

class PCI {
public:
    PCI() {}

    void Initialize();
    bool PCIExists();

    uint32_t GetDeviceClass(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF);
    char* GetClassCode(uint8_t ClassCode);
    char* GetDeviceCode(uint8_t ClassCode, uint8_t SubClass, uint8_t ProgIF);

    void checkAllBuses();
    void checkBus(uint8_t bus);

    bool EnableMSI(uint8_t bus, uint8_t device, uint8_t function, uint8_t vector);

    Array<DeviceKey> GetDevices();
private:
    Array<DeviceKey> Devices;

    uint16_t ConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
    void ConfigWriteWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
    void ConfigWriteDWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
    uint8_t ConfigReadByte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t ConfigReadDWord(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    bool deviceAlreadyFound(uint8_t bus, uint8_t device, uint8_t function);
    void addDevice(uint8_t bus, uint8_t device, uint8_t function, bool hasMSI);
    
    void checkFunction(uint8_t bus, uint8_t device, uint8_t function);
    void checkDevice(uint8_t bus, uint8_t device);
    uint16_t getVendorID(uint8_t bus, uint8_t device, uint8_t function);
    uint8_t getHeaderType(uint8_t bus, uint8_t device, uint8_t function);
};