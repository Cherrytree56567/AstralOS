#include "GenericIDEController.h"
#include "../global.h"
#include  <new>

const char* to_hstridng(uint64_t value) {
    static char buffer[19];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    do {
        uint8_t nibble = value & 0xF;
        *--ptr = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        value >>= 4;
    } while (value > 0);

    *--ptr = 'x';
    *--ptr = '0';

    return ptr;
}

const char* to_string(int64_t value) {
    static char buffer[21];
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0';

    bool isNegative = (value < 0);
    if (isNegative) value = -value;

    do {
        *--ptr = '0' + (value % 10);
        value /= 10;
    } while (value > 0);

    if (isNegative) *--ptr = '-';

    return ptr;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0"
                  : "=a"(ret)
                  : "Nd"(port));
    return ret;
}

static inline void insl(uint16_t port, void* addr, uint32_t count) {
    asm volatile ("rep insl"
                  : "+D"(addr), "+c"(count)
                  : "d"(port)
                  : "memory");
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

bool GenericIDEControllerFactory::Supports(const DeviceKey& devKey) {
    if (devKey.classCode == 0x01 && devKey.subclass == 0x01) {
        if (devKey.progIF == 0x0 || 
            devKey.progIF == 0x5 || 
            devKey.progIF == 0xA || 
            devKey.progIF == 0xF || 
            devKey.progIF == 0x80 || 
            devKey.progIF == 0x85 || 
            devKey.progIF == 0x8A || 
            devKey.progIF == 0x8F) {
                g_ds->Print("IDR drv..");
                return true;
            }
    }
    return false;
}

BlockController* GenericIDEControllerFactory::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericIDEController));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic IDE Device");
        return nullptr;
    }

    GenericIDEController* device = new(mem) GenericIDEController();
    return device;
}

void GenericIDEController::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    void* mem = _ds->malloc(sizeof(GenericIDEFactory));
    if (!mem) {
        _ds->Println("Failed to Malloc for Generic IDE Factory");
        _ds->Println(to_hstridng((uint64_t)mem));
    }

    /*
     * Don't Register the factory bc we don't want the DriverManager
     * to make more Generic AHCI Devices, since we are handling that.
    */
    GenericIDEFactory* factory = new(mem) GenericIDEFactory();

    channels[ATA_PRIMARY ].base = (devKey.bars[0] & 0xFFFFFFFC) + 0x1F0 * (!devKey.bars[0]); 
    channels[ATA_PRIMARY ].ctrl = (devKey.bars[1] & 0xFFFFFFFC) + 0x3F6 * (!devKey.bars[1]); 
    channels[ATA_SECONDARY].base = (devKey.bars[2] & 0xFFFFFFFC) + 0x170 * (!devKey.bars[2]); 
    channels[ATA_SECONDARY].ctrl = (devKey.bars[3] & 0xFFFFFFFC) + 0x376 * (!devKey.bars[3]); 
    channels[ATA_PRIMARY ].bmide = (devKey.bars[4] & 0xFFFFFFFC) + 0; 
    channels[ATA_SECONDARY].bmide = (devKey.bars[4] & 0xFFFFFFFC) + 8;

    ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    int count = 0;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            unsigned char err = 0, type = IDE_ATA, status;
            ide_devices[count].Reserved = 0;

            ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
            _ds->sleep(20); // Why 1ms tho? Is it really that fast?

            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            _ds->sleep(20);

            status = ide_read(i, ATA_REG_STATUS);
            if (status == 0) continue;

            bool errFlag = false;
            for (int t = 0; t < 1000000; t++) {
                status = ide_read(i, ATA_REG_STATUS);
                if (status & ATA_SR_ERR) {
                    errFlag = true;
                    break;
                }
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
                    break;
                }
            }

            if (errFlag) {
                uint8_t cl = ide_read(i, ATA_REG_LBA1);
                uint8_t chh = ide_read(i, ATA_REG_LBA2);
                if ((cl == 0x14 && chh == 0xEB) || (cl == 0x69 && chh == 0x96)) {
                    type = IDE_ATAPI;
                } else {
                    continue;
                }

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                _ds->sleep(1);
            }

            ide_read_buffer(i, ATA_REG_DATA, (void*) ide_buf, 128);

            ide_devices[count].Reserved     = 1;
            ide_devices[count].Type         = type;
            ide_devices[count].Channel      = i;
            ide_devices[count].Drive        = j;
            ide_devices[count].Signature    = *((unsigned short *)(ide_buf + ATA_IDENT_DEVICETYPE));
            ide_devices[count].Capabilities = *((unsigned short *)(ide_buf + ATA_IDENT_CAPABILITIES));
            ide_devices[count].CommandSets  = *((unsigned int *)(ide_buf + ATA_IDENT_COMMANDSETS));

            if (ide_devices[count].CommandSets & (1 << 26)) {
                ide_devices[count].Size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
            } else {
                ide_devices[count].Size   = *((unsigned int *)(ide_buf + ATA_IDENT_MAX_LBA));
            }

            for (int k = 0; k < 40; k += 2) {
                ide_devices[count].Model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ide_devices[count].Model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
            }
            ide_devices[count].Model[40] = 0;

            BaseDriver* device = factory->CreateDevice();

            BaseDriver* dev = this;
            DeviceKey DevK;
            DevK.bars[0] = ((uint64_t)dev >> 32);
            DevK.bars[1] = ((uint64_t)dev & 0xFFFFFFFF);
            DevK.bars[2] = count;

            device->Init(ds, DevK);
            _ds->AddDriver(device);

            count++;
        }
    }

    for (int i = 0; i < 4; i++) {
        if (ide_devices[i].Reserved == 1) {
            _ds->Print("Found ");
            if (ide_devices[i].Type == IDE_ATA) {
                _ds->Print("ATA");
            } else if (ide_devices[i].Type == IDE_ATAPI) {
                _ds->Print("ATAPI");
            }
            _ds->Print(" Drive ");
            _ds->Print(to_string(ide_devices[i].Size / 1024 / 1024 / 2));
            _ds->Print("GB - ");
            _ds->Println((const char*)ide_devices[i].Model);
      }
    }

    Initialised = true;
}

/*
 * This code is from the OSDev Wiki:
 * https://wiki.osdev.org/PCI_IDE_Controller
*/
unsigned char GenericIDEController::ide_read(unsigned char channel, unsigned char reg) {
    unsigned char result;
    if (reg > 0x07 && reg < 0x0C) {
       ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    }

    if (reg < 0x08) {
       result = inb(channels[channel].base + reg - 0x00);
    } else if (reg < 0x0C) {
       result = inb(channels[channel].base  + reg - 0x06);
    } else if (reg < 0x0E) {
       result = inb(channels[channel].ctrl  + reg - 0x0A);
    } else if (reg < 0x16) {
       result = inb(channels[channel].bmide + reg - 0x0E);
    }

    if (reg > 0x07 && reg < 0x0C) {
       ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    }
   
    return result;
}

void GenericIDEController::ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
    if (reg > 0x07 && reg < 0x0C) {
       ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
    }

    if (reg < 0x08) {
       outb(channels[channel].base  + reg - 0x00, data);
    } else if (reg < 0x0C) {
       outb(channels[channel].base  + reg - 0x06, data);
    } else if (reg < 0x0E) {
       outb(channels[channel].ctrl  + reg - 0x0A, data);
    } else if (reg < 0x16) {
       outb(channels[channel].bmide + reg - 0x0E, data);
    }

    if (reg > 0x07 && reg < 0x0C) {
       ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    }
}

void GenericIDEController::ide_read_buffer(uint8_t channel, uint8_t reg, void* buffer, uint32_t quads) {
    if (reg < 0x08) {
        insl(channels[channel].base + reg - 0x00, buffer, quads);
    } else if (reg < 0x0C) {
        insl(channels[channel].base + reg - 0x06, buffer, quads);
    } else if (reg < 0x0E) {
        insl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
    } else if (reg < 0x16) {
        insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
    }

    if (reg > 0x07 && reg < 0x0C) {
      ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
    }
}

unsigned char GenericIDEController::ide_polling(unsigned char channel, unsigned int advanced_check) {
    for (int i = 0; i < 4; i++) {
       ide_read(channel, ATA_REG_ALTSTATUS);
    }

    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY) {

    }

    if (advanced_check) {
       unsigned char state = ide_read(channel, ATA_REG_STATUS);
       
        if (state & ATA_SR_ERR) {
            return 2;
        }

        if (state & ATA_SR_DF) {
            return 1;
        }

        if ((state & ATA_SR_DRQ) == 0) {
            return 3;
        }
   }

   return 0;
}

/*
 * Look how clean it looks now!!!
 * Who even wrote that mess before???
 * ToT
 * 
 * tbh, mine looks a bit bad now.
*/
unsigned char GenericIDEController::ide_print_error(uint8_t drive, unsigned char err) {
    if (err == 0) {
       return err;
    }

    _ds->Print("[IDE] ");
    if (err == 1) {
        _ds->Println("Device Fault"); 
        err = 19;
    } else if (err == 2) {
        unsigned char st = ide_read(ide_devices[drive].Channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF) {
            _ds->Println("No Address Mark Found");
            err = 7;
        }

        if ((st & ATA_ER_TK0NF) || (st & ATA_ER_MCR) || (st & ATA_ER_MC)) {
            _ds->Println("No Media or Media Error");
            err = 3;
        }

        if (st & ATA_ER_ABRT) {
            _ds->Println("Command Aborted");
            err = 20;
        }

        if (st & ATA_ER_IDNF) {
            _ds->Println("ID mark not Found");
            err = 21;
        }

        if (st & ATA_ER_UNC) {
            _ds->Println("Uncorrectable Data Error");
            err = 22;
        }

        if (st & ATA_ER_BBK) {
            _ds->Println("Bad Sectors");
            err = 13;
        }
    } else if (err == 3) {
        _ds->Println("Reads Nothing");
        err = 23;
    } else if (err == 4) {
        _ds->Println("Write Protected");
        err = 8;
    }
    _ds->Print("- [");

    if (ide_devices[drive].Channel == ATA_PRIMARY) {
        _ds->Print("Primary");
    } else if (ide_devices[drive].Channel == ATA_SECONDARY) {
        _ds->Print("Secondary");
    }

    _ds->Print(" ");

    if (ide_devices[drive].Drive == ATA_MASTER) {
        _ds->Print("Master");
    } else if (ide_devices[drive].Drive == ATA_SLAVE) {
        _ds->Print("Slave");
    }

    _ds->Print("] ");
    _ds->Println((const char*)ide_devices[drive].Model);

   return err;
}

unsigned char GenericIDEController::ide_ata_access(uint8_t drive, unsigned char direction, unsigned int lba, unsigned char numsects, void* buffer) {
    unsigned char lba_mode, cmd;
    unsigned char lba_io[6];
    uint32_t channel = ide_devices[drive].Channel;
    uint32_t slavebit = ide_devices[drive].Drive;
    uint16_t bus = channels[channel].base;
    unsigned char head;

    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY) {

    }

    if (lba >= 0x10000000) {
        lba_mode = 2;
        lba_io[0] = (lba & 0xFF);
        lba_io[1] = (lba >> 8) & 0xFF;
        lba_io[2] = (lba >> 16) & 0xFF;
        lba_io[3] = (lba >> 24) & 0xFF;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = 0;
    } else {
        lba_mode = 1;
        lba_io[0] = (lba & 0xFF);
        lba_io[1] = (lba >> 8) & 0xFF;
        lba_io[2] = (lba >> 16) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba >> 24) & 0xF;
    }

    if (lba_mode == 1) {
        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head);
    } else {
        ide_write(channel, ATA_REG_HDDEVSEL, 0x40 | (slavebit << 4));
    }

    ide_write(channel, ATA_REG_SECCOUNT0, numsects);
    ide_write(channel, ATA_REG_LBA0, lba_io[0]);
    ide_write(channel, ATA_REG_LBA1, lba_io[1]);
    ide_write(channel, ATA_REG_LBA2, lba_io[2]);

    if (direction == 0) {
        cmd = (lba_mode == 2) ? ATA_CMD_READ_PIO_EXT : ATA_CMD_READ_PIO;
    } else {
        cmd = (lba_mode == 2) ? ATA_CMD_WRITE_PIO_EXT : ATA_CMD_WRITE_PIO;
    }

    ide_write(channel, ATA_REG_COMMAND, cmd);

    uint16_t* buf = (uint16_t*)buffer;
    for (unsigned char i = 0; i < numsects; i++) {
        if (direction == 0) {
            if (ide_polling(channel, 1)) {
                return 1;
            }
            for (int w = 0; w < 256; w++) {
                buf[w] = inw(bus);
            }
        } else {
            if (ide_polling(channel, 0)) return 1;
            for (int w = 0; w < 256; w++) {
                outw(bus, buf[w]);
            }
            ide_write(channel, ATA_REG_COMMAND, (lba_mode == 2) ? ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH);
            ide_polling(channel, 0);
        }
        buf += 256;
    }
    return 0;
}

void GenericIDEController::ide_wait_irq() {
   while (!ide_irq_invoked)
      ;
   ide_irq_invoked = 0;
}

void GenericIDEController::ide_irq() {
   ide_irq_invoked = 1;
}

unsigned char GenericIDEController::ide_atapi_read(uint8_t drive, unsigned int lba, unsigned char numsects, void* buffer) {
    unsigned int channel = ide_devices[drive].Channel;
    unsigned int slavebit = ide_devices[drive].Drive;
    unsigned int bus = channels[channel].base;
    unsigned int words = 1024;
    unsigned char err;

    ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);

    atapi_packet[0] = ATAPI_CMD_READ;
    atapi_packet[1] = 0x0;
    atapi_packet[2] = (lba >> 24) & 0xFF;
    atapi_packet[3] = (lba >> 16) & 0xFF;
    atapi_packet[4] = (lba >> 8) & 0xFF;
    atapi_packet[5] = (lba >> 0) & 0xFF;
    atapi_packet[6] = 0x0;
    atapi_packet[7] = 0x0;
    atapi_packet[8] = 0x0;
    atapi_packet[9] = numsects;
    atapi_packet[10] = 0x0;
    atapi_packet[11] = 0x0;

    ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);

    for(int i = 0; i < 4; i++) {
        ide_read(channel, ATA_REG_ALTSTATUS);
    }

    ide_write(channel, ATA_REG_FEATURES, 0);

    ide_write(channel, ATA_REG_LBA1, (words * 2) & 0xFF);
    ide_write(channel, ATA_REG_LBA2, (words * 2) >> 8);

    ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);

    if (err = ide_polling(channel, 1)) {
        return err;
    }

    asm volatile("rep outsw" : : "c"(6), "d"(bus), "S"(atapi_packet));

    uint16_t* buf16 = (uint16_t*)buffer;
    for (int i = 0; i < numsects; i++) {
        ide_wait_irq();
        if ((err = ide_polling(channel, 1))) {
            return err;
        }

        asm volatile(
            "rep insw"
            : "=D"(buf16)
            : "c"(words), "d"(bus), "D"(buf16)
            : "memory"
        );

        buf16 += words;
    }

    return 0;
}

bool GenericIDEController::ReadSector(uint8_t drive, uint64_t lba, void* buffer) {
    if (drive > 3 || ide_devices[drive].Reserved == 0) {
        _ds->Println("[IDE] Drive Not Found");
        return false;
    }

    if ((lba >= ide_devices[drive].Size) && ide_devices[drive].Type == IDE_ATA) {
        _ds->Println("[IDE] Out of bounds");
        return false;
    }

    unsigned char err = 0;
    if (ide_devices[drive].Type == IDE_ATA) {
        err = ide_ata_access(drive, ATA_READ, lba, 1, buffer);
    } else if (ide_devices[drive].Type == IDE_ATAPI) {
        err = ide_atapi_read(drive, lba, 1, buffer);
    }

    ide_print_error(drive, err);
    return true;
}

bool GenericIDEController::WriteSector(uint8_t drive, uint64_t lba, void* buffer) {
    if (drive > 3 || ide_devices[drive].Reserved == 0) {
        _ds->Println("[IDE] Drive Not Found");
        return false;
    }

    if ((lba >= ide_devices[drive].Size) && ide_devices[drive].Type == IDE_ATA) {
        _ds->Println("[IDE] Out of bounds");
        return false;
    }

    unsigned char err = 0;
    if (ide_devices[drive].Type == IDE_ATA) {
        err = ide_ata_access(drive, ATA_WRITE, lba, 1, buffer);
    } else if (ide_devices[drive].Type == IDE_ATAPI) {
        err = 4;
    }

    ide_print_error(drive, err);
}

uint64_t GenericIDEController::SectorCount(uint8_t drive) const {
    return ide_devices[drive].Size;
}

uint32_t GenericIDEController::SectorSize(uint8_t drive) const {
    if (ide_devices[drive].Type == IDE_ATA) {
        return 512;
    } else if (ide_devices[drive].Type == IDE_ATAPI) {
        return 2048;
    }
    return 0;
}

void* GenericIDEController::GetInternalBuffer(uint8_t drive) {
    return (void*)ide_buf;
}

uint8_t GenericIDEController::GetClass() {
    return devKey.classCode;
}

uint8_t GenericIDEController::GetSubClass() {
    return devKey.subclass;
}

uint8_t GenericIDEController::GetProgIF() {
    return devKey.progIF;
}

/*
 * Usually, You would have a database to get them from, 
 * but seeing as the kernel doesn't really care about
 * the name of the device, we can just hardcode these
 * for now.
*/
const char* GenericIDEController::name(uint8_t drive) const {
    uint16_t vendor = _ds->ConfigReadWord(devKey.bus, devKey.device, devKey.function, 0x00);
    uint16_t device = _ds->ConfigReadWord(devKey.bus, devKey.device, devKey.function, 0x02);
    if (vendor == 0x8086) {
        return "Intel IDE Controller";
    } else if (vendor == 0x80EE) {
        return "VirtualBox IDE Controller";
    }
}

const char* GenericIDEController::DriverName() const {
    return "Generic IDE Driver";
}