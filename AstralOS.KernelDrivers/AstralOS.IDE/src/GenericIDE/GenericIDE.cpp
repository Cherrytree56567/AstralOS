#include "GenericIDE.h"
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

bool GenericIDE::Supports(const DeviceKey& devKey) {
    if (devKey.classCode == 0x01 && devKey.subclass == 0x01) {
        return true;
    }
    return false;
}

BlockDevice* GenericIDE::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericIDEDevice));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic IDE Device");
        return nullptr;
    }

    GenericIDEDevice* device = new(mem) GenericIDEDevice();
    return device;
}

void GenericIDEDevice::Init(DriverServices& ds, DeviceKey& devKey) {
    if (Initialised) {
        return;
    }

    _ds = &ds;
    devKey = devKey;
    _ds->Println("GenericIDE Driver Initialised!");

    Initialised = true;
}

/*
 * This code is from the OSDev Wiki:
 * https://wiki.osdev.org/PCI_IDE_Controller
*/
unsigned char GenericIDEDevice::ide_read(unsigned char channel, unsigned char reg) {
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

void GenericIDEDevice::ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
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

void GenericIDEDevice::ide_read_buffer(uint8_t channel, uint8_t reg, void* buffer, uint32_t quads) {
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

unsigned char GenericIDEDevice::ide_polling(unsigned char channel, unsigned int advanced_check) {
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
 * tbh, mine is looks a bit bad now.
*/
unsigned char GenericIDEDevice::ide_print_error(unsigned int drive, unsigned char err) {
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

bool GenericIDEDevice::ReadSector(uint64_t lba, void* buffer) {

}

bool GenericIDEDevice::WriteSector(uint64_t lba, const void* buffer) {

}

uint64_t GenericIDEDevice::SectorCount() const {

}

uint32_t GenericIDEDevice::SectorSize() const {

}

void* GenericIDEDevice::GetInternalBuffer() {

}

uint8_t GenericIDEDevice::GetClass() {
    return devKey->classCode;
}

uint8_t GenericIDEDevice::GetSubClass() {
    return devKey->subclass;
}

uint8_t GenericIDEDevice::GetProgIF() {
    return devKey->progIF;
}

/*
 * Usually, You would have a database to get them from, 
 * but seeing as the kernel doesn't really care about
 * the name of the device, we can just hardcode these
 * for now.
*/
const char* GenericIDEDevice::name() const {
    uint16_t vendor = _ds->ConfigReadWord(devKey->bus, devKey->device, devKey->function, 0x00);
    uint16_t device = _ds->ConfigReadWord(devKey->bus, devKey->device, devKey->function, 0x02);
    if (vendor == 0x8086) {
        return "Intel IDE Controller";
    } else if (vendor == 0x80EE) {
        return "VirtualBox IDE Controller";
    }
}

const char* GenericIDEDevice::DriverName() const {
    return "Generic IDE Driver";
}