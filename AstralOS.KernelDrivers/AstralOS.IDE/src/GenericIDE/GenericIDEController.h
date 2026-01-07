#pragma once
#define DRIVER
#include "../PCI.h"
#include "../DriverServices.h"
#include "GenericIDE.h"

/*
 * These enums are from the OSDev Wiki:
 * https://wiki.osdev.org/PCI_IDE_Controller
*/
enum ATA_Status {
    ATA_SR_BSY = 0x80,  // Busy
    ATA_SR_DRDY = 0x40, // Drive ready
    ATA_SR_DF = 0x20,   // Drive write fault
    ATA_SR_DSC = 0x10,  // Drive seek complete
    ATA_SR_DRQ = 0x08,  // Data request ready
    ATA_SR_CORR = 0x04, // Corrected data
    ATA_SR_IDX = 0x02,  // Index
    ATA_SR_ERR = 0x01   // Error
};

enum ATA_Errors {
    ATA_ER_BBK = 0x80,      // Bad block
    ATA_ER_UNC = 0x40,      // Uncorrectable data
    ATA_ER_MC = 0x20,       // Media changed
    ATA_ER_IDNF = 0x10,     // ID mark not found
    ATA_ER_MCR = 0x08,      // Media change request
    ATA_ER_ABRT = 0x04,     // Command aborted
    ATA_ER_TK0NF = 0x02,    // Track 0 not found
    ATA_ER_AMNF = 0x01,     // No address mark
};

enum ATA_Commands { 
    ATA_CMD_READ_PIO = 0x20,
    ATA_CMD_READ_PIO_EXT = 0x24,
    ATA_CMD_READ_DMA = 0xC8,
    ATA_CMD_READ_DMA_EXT = 0x25,
    ATA_CMD_WRITE_PIO = 0x30,
    ATA_CMD_WRITE_PIO_EXT = 0x34,
    ATA_CMD_WRITE_DMA = 0xCA,
    ATA_CMD_WRITE_DMA_EXT = 0x35,
    ATA_CMD_CACHE_FLUSH = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_PACKET = 0xA0,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    ATA_CMD_IDENTIFY = 0xEC
};

enum ATAPI_Commands {
    ATAPI_CMD_READ = 0xA8,
    ATAPI_CMD_EJECT = 0x1B
};

enum ATA_IDENT {
    ATA_IDENT_DEVICETYPE = 0,
    ATA_IDENT_CYLINDERS = 2,
    ATA_IDENT_HEADS = 6,
    ATA_IDENT_SECTORS = 12,
    ATA_IDENT_SERIAL = 20,
    ATA_IDENT_MODEL = 54,
    ATA_IDENT_CAPABILITIES = 98,
    ATA_IDENT_FIELDVALID = 106,
    ATA_IDENT_MAX_LBA = 120,
    ATA_IDENT_COMMANDSETS = 164,
    ATA_IDENT_MAX_LBA_EXT = 200
};

enum IDE_Interface {
    IDE_ATA = 0x00,
    IDE_ATAPI = 0x01
};

enum IDE_Drive {
    ATA_MASTER = 0x00,
    ATA_SLAVE = 0x01
};

enum ATA_Registry {
    ATA_REG_DATA = 0x00,
    ATA_REG_ERROR = 0x01,
    ATA_REG_FEATURES = 0x01,
    ATA_REG_SECCOUNT0 = 0x02,
    ATA_REG_LBA0 = 0x03,
    ATA_REG_LBA1 = 0x04,
    ATA_REG_LBA2 = 0x05,
    ATA_REG_HDDEVSEL = 0x06,
    ATA_REG_COMMAND = 0x07,
    ATA_REG_STATUS = 0x07,
    ATA_REG_SECCOUNT1 = 0x08,
    ATA_REG_LBA3 = 0x09,
    ATA_REG_LBA4 = 0x0A,
    ATA_REG_LBA5 = 0x0B,
    ATA_REG_CONTROL = 0x0C,
    ATA_REG_ALTSTATUS = 0x0C,
    ATA_REG_DEVADDRESS = 0x0D
};

enum ATA_Channel {
    ATA_PRIMARY = 0x00,
    ATA_SECONDARY = 0x01
};

enum ATA_Direction {
    ATA_READ = 0x00,
    ATA_WRITE = 0x01
};

struct IDEChannelRegisters {
   unsigned short base;  // I/O Base.
   unsigned short ctrl;  // Control Base
   unsigned short bmide; // Bus Master IDE
   unsigned char  nIEN;  // nIEN (No Interrupt);
};

struct ide_device {
   unsigned char  Reserved;    // 0 (Empty) or 1 (This Drive really exists).
   unsigned char  Channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
   unsigned char  Drive;       // 0 (Master Drive) or 1 (Slave Drive).
   unsigned short Type;        // 0: ATA, 1:ATAPI.
   unsigned short Signature;   // Drive Signature
   unsigned short Capabilities;// Features.
   unsigned int   CommandSets; // Command Sets Supported.
   unsigned int   Size;        // Size in Sectors.
   unsigned char  Model[41];   // Model in string.
};

class GenericIDEControllerFactory : public BlockControllerFactory {
public:
    virtual ~GenericIDEControllerFactory() override {}
    virtual bool Supports(const DeviceKey& devKey) override;
    virtual BlockController* CreateDevice() override;
};

class GenericIDEController : public BlockController {
public:
    virtual void Init(DriverServices& ds, DeviceKey& devKey) override;
    virtual bool ReadSector(uint8_t drive, uint64_t lba, void* buffer) override;
    virtual bool WriteSector(uint8_t drive, uint64_t lba, void* buffer) override;

    virtual uint64_t SectorCount(uint8_t drive) const override;
    virtual uint32_t SectorSize(uint8_t drive) const override;
    virtual void* GetInternalBuffer(uint8_t drive) override;

    virtual uint8_t GetClass() override;
    virtual uint8_t GetSubClass() override;
    virtual uint8_t GetProgIF() override;
    virtual const char* name(uint8_t drive) const override;
    virtual const char* DriverName() const override;
private:
    unsigned char ide_read(unsigned char channel, unsigned char reg);
    void ide_write(unsigned char channel, unsigned char reg, unsigned char data);
    void ide_read_buffer(uint8_t channel, uint8_t reg, void* buffer, uint32_t quads);
    unsigned char ide_polling(unsigned char channel, unsigned int advanced_check);
    unsigned char ide_print_error(uint8_t drive, unsigned char err);
    unsigned char ide_ata_access(uint8_t drive, unsigned char direction, unsigned int lba, unsigned char numsects, void* buffer);
    unsigned char ide_atapi_read(uint8_t drive, unsigned int lba, unsigned char numsects, void* buffer);

    void ide_irq();
    void ide_wait_irq();

    DriverServices* _ds = nullptr;
    DeviceKey devKey;
    bool Initialised = false;

    unsigned char ide_buf[2048] = {0};
    volatile unsigned char ide_irq_invoked;
    unsigned char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    IDEChannelRegisters channels[2];
    ide_device ide_devices[4];
};