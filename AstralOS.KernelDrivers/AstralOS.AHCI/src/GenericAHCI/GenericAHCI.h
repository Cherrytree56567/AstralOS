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
 * From IPXE:
 * https://dox.ipxe.org/ata_8h.html
*/
#define ATA_CMD_IDENTIFY 0xec

/*
 * From Huihoo:
 * https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2linux_2ata_8h.html
*/
#define ATA_CMD_PACKET 0xA0

/*
 * From singlix:
 * https://www.singlix.com/trdos/archive/code_archive/IDE_ATA_ATAPI_Tutorial.pdf
*/
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_READ_PIO_EXT 0x24
#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET 0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY 0xEC

#define ATAPI_CMD_READ 0xA8
#define ATAPI_CMD_EJECT 0x1B

#define ATAPI_CMD_IDENTIFY 0xa1

/*
 * From acess2:
 * https://gitlab.ucc.gu.uwa.edu.au/tpg/acess2/-/blob/00a16a4e22cd2445414db9dc1ae0dfc99e69d584/KernelLand/Modules/Storage/AHCI/ata.h
*/
#define ATA_STATUS_BSY 0x80
#define ATA_STATUS_DRQ 0x08

/*
 * From Quinn OS:
 * https://git.quinnos.com/quinn-os/quinn-os/raw/commit/1046606ae29b2faa6354f41a5ea941c673a35fa5/ahci.c
*/
#define HBA_PxIS_TFES (1 << 30)

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

/*
 * From Tianocore EDKII
 * https://github.com/tianocore/edk2/blob/master/MdePkg/Include/IndustryStandard/Atapi.h#L78
*/
struct ATA_IDENTIFY_DATA {
    uint16_t config;
    uint16_t obsolete_1;
    uint16_t specific_config;
    uint16_t obsolete_3;
    uint16_t retired_4_5[2];
    uint16_t obsolete_6;
    uint16_t cfa_reserved_7_8[2];
    uint16_t retired_9;
    char SerialNo[20];
    uint16_t retired_20_21[2];
    uint16_t obsolete_22;
    char FirmwareVer[8];
    char ModelName[40];
    uint16_t multi_sector_cmd_max_sct_cnt;
    uint16_t trusted_computing_support;
    uint16_t capabilities_49;
    uint16_t capabilities_50;
    uint16_t obsolete_51_52[2];
    uint16_t field_validity;
    uint16_t obsolete_54_58[5];
    uint16_t multi_sector_setting;
    uint16_t user_addressable_sectors_lo;
    uint16_t user_addressable_sectors_hi;
    uint16_t obsolete_62;
    uint16_t multi_word_dma_mode;
    uint16_t advanced_pio_modes;
    uint16_t min_multi_word_dma_cycle_time;
    uint16_t rec_multi_word_dma_cycle_time;
    uint16_t min_pio_cycle_time_without_flow_control;
    uint16_t min_pio_cycle_time_with_flow_control;
    uint16_t additional_supported;
    uint16_t reserved_70;
    uint16_t reserved_71_74[4];
    uint16_t queue_depth;
    uint16_t serial_ata_capabilities;
    uint16_t reserved_77;
    uint16_t serial_ata_features_supported;
    uint16_t serial_ata_features_enabled;
    uint16_t major_version_no;
    uint16_t minor_version_no;
    uint16_t command_set_supported_82;
    uint16_t command_set_supported_83;
    uint16_t command_set_feature_extn;
    uint16_t command_set_feature_enb_85;
    uint16_t command_set_feature_enb_86;
    uint16_t command_set_feature_default;
    uint16_t ultra_dma_mode;
    uint16_t time_for_security_erase_unit;
    uint16_t time_for_enhanced_security_erase_unit;
    uint16_t advanced_power_management_level;
    uint16_t master_password_identifier;
    uint16_t hardware_configuration_test_result;
    uint16_t obsolete_94;
    uint16_t stream_minimum_request_size;
    uint16_t streaming_transfer_time_for_dma;
    uint16_t streaming_access_latency_for_dma_and_pio;
    uint16_t streaming_performance_granularity[2];
    uint16_t maximum_lba_for_48bit_addressing[4];
    uint16_t streaming_transfer_time_for_pio;
    uint16_t max_no_of_512byte_blocks_per_data_set_cmd;
    uint16_t phy_logic_sector_support;
    uint16_t interseek_delay_for_iso7779;
    uint16_t world_wide_name[4];
    uint16_t reserved_for_128bit_wwn_112_115[4];
    uint16_t reserved_for_technical_report;
    uint16_t logic_sector_size_lo;
    uint16_t logic_sector_size_hi;
    uint16_t features_and_command_sets_supported_ext;
    uint16_t features_and_command_sets_enabled_ext;
    uint16_t reserved_121_126[6];
    uint16_t obsolete_127;
    uint16_t security_status;
    uint16_t vendor_specific_129_159[31];
    uint16_t cfa_power_mode;
    uint16_t reserved_for_compactflash_161_167[7];
    uint16_t device_nominal_form_factor;
    uint16_t is_data_set_cmd_supported;
    char additional_product_identifier[8];
    uint16_t reserved_174_175[2];
    char media_serial_number[60];
    uint16_t sct_command_transport;
    uint16_t reserved_207_208[2];
    uint16_t alignment_logic_in_phy_blocks;
    uint16_t write_read_verify_sector_count_mode3[2];
    uint16_t verify_sector_count_mode2[2];
    uint16_t nv_cache_capabilities;
    uint16_t nv_cache_size_in_logical_block_lsw;
    uint16_t nv_cache_size_in_logical_block_msw;
    uint16_t nominal_media_rotation_rate;
    uint16_t reserved_218;
    uint16_t nv_cache_options;
    uint16_t write_read_verify_mode;
    uint16_t reserved_221;
    uint16_t transport_major_revision_number;
    uint16_t transport_minor_revision_number;
    uint16_t reserved_224_229[6];
    uint64_t extended_no_of_addressable_sectors;
    uint16_t min_number_per_download_microcode_mode3;
    uint16_t max_number_per_download_microcode_mode3;
    uint16_t reserved_236_254[19];
    uint16_t integrity_word;
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

	/*
	 * Not OSDev Wiki code
	*/
	bool ahci_send_cmd(HBA_PORT* port, FIS_H2D* fis, void* buffer, uint32_t buf_size, uint16_t prdtl = 1, bool write = false);
	bool cd_send_cmd(HBA_PORT* port, FIS_H2D* fis, void* buffer, uint32_t buf_size, uint8_t* atapi_packet, size_t packet_len);

    DriverServices* _ds = nullptr;
    DeviceKey devKey;
	ATA_IDENTIFY_DATA* portInfo[32];
	HBA_MEM* hba = nullptr;
	uint8_t drive = 0;
	bool driveSet = false;
};