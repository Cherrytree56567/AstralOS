#pragma once
#define DRIVER
#include "../PCI.h"
#include "../DriverServices.h"
#include <cstdint>

#define	SATA_SIG_ATA 0x00000101	// SATA drive
#define	SATA_SIG_ATAPI 0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB 0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define	AHCI_BASE 0x400000

#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR 0x4000
#define HBA_PxCMD_CR 0x8000

/*
 * Structs from the OSDev Wiki:
 * https://wiki.osdev.org/AHCI
*/
enum FIS_TYPE {
	FIS_TYPE_REG_H2D = 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H = 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT = 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP = 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA = 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST = 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP = 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS = 0xA1,	// Set device bits FIS - device to host
};

struct FIS_DEV_BITS {
    uint8_t fis_type;

    uint8_t pmport : 4;
    uint8_t rsv0 : 4;

    uint8_t status;
    uint8_t error;

    uint32_t notification;

    uint32_t rsv1[2];
};

struct FIS_H2D {
	uint8_t fis_type;

	uint8_t pmport : 4;
	uint8_t rsv0 : 3;
	uint8_t c : 1;

	uint8_t command;
	uint8_t featurel;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t featureh;

	uint8_t countl;
	uint8_t counth;
	uint8_t icc;
	uint8_t control;

	uint8_t rsv1[4];
};

struct FIS_D2H {
	uint8_t fis_type;

	uint8_t pmport:4;
	uint8_t rsv0:2;
	uint8_t i:1;
	uint8_t rsv1:1;

	uint8_t status;
	uint8_t error;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t rsv2;

	uint8_t countl;
	uint8_t counth;
	uint8_t rsv3[2];

	uint8_t rsv4[4];
};

struct FIS_DATA {
	uint8_t fis_type;

	uint8_t pmport : 4;
	uint8_t rsv0 : 4;

	uint8_t rsv1[2];

	uint32_t data[1];
};

struct FIS_PIO {
	uint8_t fis_type;

	uint8_t pmport : 4;
	uint8_t rsv0 : 1;
	uint8_t d : 1;
	uint8_t i : 1;
	uint8_t rsv1 : 1;

	uint8_t status;
	uint8_t error;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t rsv2;

	uint8_t countl;
	uint8_t counth;
	uint8_t rsv3;
	uint8_t e_status;

	uint16_t tc;
	uint8_t rsv4[2];
};

struct FIS_DMA {
	uint8_t fis_type;
	uint8_t pmport : 4;
	uint8_t rsv0 : 1;
	uint8_t d : 1;
	uint8_t i : 1;
	uint8_t a : 1;
    uint8_t rsved[2];
    uint64_t DMAbufferID;
    uint32_t rsvd;
    uint32_t DMAbufOffset;
    uint32_t TransferCount;
    uint32_t resvd;
};

struct HBA_PORT {
	uint32_t clb;		// 0x00, command list base address, 1K-byte aligned
	uint32_t clbu;		// 0x04, command list base address upper 32 bits
	uint32_t fb;		// 0x08, FIS base address, 256-byte aligned
	uint32_t fbu;		// 0x0C, FIS base address upper 32 bits
	uint32_t is;		// 0x10, interrupt status
	uint32_t ie;		// 0x14, interrupt enable
	uint32_t cmd;		// 0x18, command and status
	uint32_t rsv0;		// 0x1C, Reserved
	uint32_t tfd;		// 0x20, task file data
	uint32_t sig;		// 0x24, signature
	uint32_t ssts;		// 0x28, SATA status (SCR0:SStatus)
	uint32_t sctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32_t serr;		// 0x30, SATA error (SCR1:SError)
	uint32_t sact;		// 0x34, SATA active (SCR3:SActive)
	uint32_t ci;		// 0x38, command issue
	uint32_t sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fbs;		// 0x40, FIS-based switch control
	uint32_t rsv1[11];	// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	// 0x70 ~ 0x7F, vendor specific
};

struct HBA_MEM {
	uint32_t cap;
	uint32_t ghc;
	uint32_t is;
	uint32_t pi;
	uint32_t vs;
	uint32_t ccc_ctl;
	uint32_t ccc_pts;
	uint32_t em_loc;
	uint32_t em_ctl;
	uint32_t cap2;
	uint32_t bohc;

	uint8_t rsv[0xA0-0x2C];

	uint8_t vendor[0x100-0xA0];

	HBA_PORT ports[1];
};

struct HBA_FIS {
	FIS_DMA	dsfis;
	uint8_t pad0[4];

	FIS_PIO	psfis;
	uint8_t pad1[12];

	FIS_D2H	rfis;
	uint8_t pad2[4];

	FIS_DEV_BITS sdbfis;
	
	uint8_t ufis[64];

	uint8_t rsv[0x100-0xA0];
};

struct HBA_CMD {
	uint8_t cfl : 5;
	uint8_t a : 1;
	uint8_t w : 1;
	uint8_t p : 1;

	uint8_t r : 1;
	uint8_t b : 1;
	uint8_t c : 1;
	uint8_t rsv0 : 1;
	uint8_t pmp : 4;

	uint16_t prdtl;

	volatile uint32_t prdbc;

	uint32_t ctba;
	uint32_t ctbau;

	uint32_t rsv1[4];
};

struct HBA_PRDT_ENTRY {
	uint32_t dba;
	uint32_t dbau;
	uint32_t rsv0;

	uint32_t dbc : 22;
	uint32_t rsv1 : 9;
	uint32_t i : 1;
};

struct HBA_CMD_TBL {
	uint8_t cfis[64];
	uint8_t acmd[16];
	uint8_t rsv[48];
	HBA_PRDT_ENTRY prdt_entry[1];
};

class GenericAHCI : public BlockDeviceFactory {
public:
    virtual ~GenericAHCI() override {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual BlockDevice* CreateDevice() override;
};

class GenericAHCIDevice : public BlockDevice {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override;
    virtual bool ReadSector(uint64_t lba, void* buffer) override;
    virtual bool WriteSector(uint64_t lba, void* buffer) override;
    virtual bool SetDrive(uint8_t drive) override;

    virtual uint64_t SectorCount() const override;
    virtual uint32_t SectorSize() const override;
    virtual void* GetInternalBuffer() override;

    virtual uint8_t GetClass() override;
    virtual uint8_t GetSubClass() override;
    virtual uint8_t GetProgIF() override;
    virtual const char* name() const override;
    virtual const char* DriverName() const override;
private:
    void probe_port(HBA_MEM *abar);
    int check_type(HBA_PORT *port);
    void port_rebase(HBA_PORT *port, int portno);
    void start_cmd(HBA_PORT *port);
    void stop_cmd(HBA_PORT *port);

    DriverServices* _ds = nullptr;
    DeviceKey devKey;
};