#include "GenericAHCIController.h"
#include "../global.h"
#include <new>

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

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];

    return dest;
}

bool GenericAHCIControllerFactory::Supports(const DeviceKey& devKey) {
    if (devKey.classCode == 0x01 && devKey.subclass == 0x6 && devKey.progIF == 0x1) {
        return true;
    }
    return false;
}

BlockController* GenericAHCIControllerFactory::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericAHCIController));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic AHCI Device");
        return nullptr;
    }

    GenericAHCIController* device = new(mem) GenericAHCIController();
    return device;
}

void* memset(void* dest, uint8_t value, size_t num) {
    uint8_t* ptr = (uint8_t*)dest;
    for (size_t i = 0; i < num; i++) {
        ptr[i] = value;
    }
    return dest;
}

void GenericAHCIController::probe_port(HBA_MEM *abar) {
	uint32_t pi = abar->pi;
	int i = 0;
	while (i < 32) {
		if (pi & 1) {
			int dt = check_type(&abar->ports[i]);

			if (dt == AHCI_DEV_SATA) {
				_ds->Print("SATA drive found at port ");
				_ds->Println(to_hstridng(i));
			} else if (dt == AHCI_DEV_SATAPI) {
				_ds->Print("SATAPI drive found at port ");
				_ds->Println(to_hstridng(i));
			} else if (dt == AHCI_DEV_SEMB) {
				_ds->Print("SEMB drive found at port ");
				_ds->Println(to_hstridng(i));
			} else if (dt == AHCI_DEV_PM) {
				_ds->Print("PM drive found at port ");
				_ds->Println(to_hstridng(i));
			} else {
				_ds->Print("No drive found at port ");
				_ds->Println(to_hstridng(i));
			}
		}

		pi >>= 1;
		i++;
	}
}

int GenericAHCIController::check_type(HBA_PORT *port) {
	uint32_t ssts = port->ssts;

	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != HBA_PORT_DET_PRESENT) {
		return AHCI_DEV_NULL;
    } if (ipm != HBA_PORT_IPM_ACTIVE) {
		return AHCI_DEV_NULL;
    }

	switch (port->sig) {
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:
            return AHCI_DEV_SEMB;
        case SATA_SIG_PM:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA;
	}
}

void GenericAHCIController::port_rebase(HBA_PORT *port, int portno) {
	stop_cmd(port);

	/*
     * Command list offset: 1K*portno
	 * Command list entry size = 32
	 * Command list entry maxim count = 32
	 * Command list maxim size = 32*32 = 1K per port
    */
	port->clb = AHCI_BASE + (portno << 10);
	port->clbu = 0;
	memset((void*)(port->clb), 0, 1024);

	/*
     * FIS offset: 32K+256*portno
	 * FIS entry size = 256 bytes per port
    */
	port->fb = AHCI_BASE + (32 << 10) + (portno << 8);
	port->fbu = 0;
	memset((void*)(port->fb), 0, 256);

	/*
     * Command table offset: 40K + 8K*portno
	 * Command table size = 256*32 = 8K per port
    */
	HBA_CMD *cmdheader = (HBA_CMD*)(port->clb);
	for (int i = 0; i < 32; i++) {
        /*
         * 8 prdt entries per command table
         * 256 bytes per command table, 64 + 16 + 48 + 16 * 8
         * Command table offset: 40K + 8K * portno + cmdheader_index * 256
        */
		cmdheader[i].prdtl = 8;
		cmdheader[i].ctba = AHCI_BASE + (40 << 10) + (portno << 13) + (i << 8);
		cmdheader[i].ctbau = 0;
		memset((void*)cmdheader[i].ctba, 0, 256);
	}

	start_cmd(port);
}

void GenericAHCIController::start_cmd(HBA_PORT *port) {
	/*
     * Wait until CR (bit15) is cleared
    */
	while (port->cmd & HBA_PxCMD_CR);

	/*
     * Set FRE (bit4) and ST (bit0)
    */
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST; 
}

void GenericAHCIController::stop_cmd(HBA_PORT *port) {
	/*
     * Clear ST (bit0)
    */
	port->cmd &= ~HBA_PxCMD_ST;

	/*
     * Clear FRE (bit4)
    */
	port->cmd &= ~HBA_PxCMD_FRE;

	/*
     * Wait until FR (bit14), CR (bit15) are cleared
    */
	while (1) {
		if (port->cmd & HBA_PxCMD_FR) {
			continue;
        }

		if (port->cmd & HBA_PxCMD_CR) {
			continue;
        }

		break;
	}
}

bool GenericAHCIController::ahci_send_cmd(HBA_PORT* port, FIS_H2D* fis, void* buffer, uint32_t buf_size, uint16_t prdtl, bool write) {
    uint32_t slot = 0xFFFFFFFF;
    uint32_t timer = 100000;
    while (slot == 0xFFFFFFFF) {
        uint32_t slots = port->sact | port->ci;
        for (int i = 0; i < 32; i++) {
            if ((slots & (1 << i)) == 0) {
                slot = i;
                break;
            }
        }
        _ds->sleep(5);
        if (timer-- == 0) {
            return false;
        }
    }

    int8_t status = port->tfd;

    if (status & ATA_STATUS_BSY) {
        _ds->Println("Device is busy");
    }
    if (status & ATA_STATUS_DRQ) {
        _ds->Println("Device requesting data");
    }

    port->is = (uint32_t)-1;

    HBA_CMD* cmdheader = (HBA_CMD*)port->clb;
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(0xFFFFFFFF00000000 + cmdheader[slot].ctba);

    cmdheader[slot].cfl = sizeof(FIS_H2D) / sizeof(uint32_t);
    if (write) {
        cmdheader[slot].w = 1;
    } else {
        cmdheader[slot].w = 0;
    }
    cmdheader[slot].c = 0;
    cmdheader[slot].prdtl = prdtl;
    cmdheader[slot].prdbc = 0;

    cmdtbl->prdt_entry[0].dba = (uint32_t)((uintptr_t)buffer & 0xFFFFFFFF);
    cmdtbl->prdt_entry[0].dbau = (uint32_t)(((uintptr_t)buffer >> 32) & 0xFFFFFFFF);
    cmdtbl->prdt_entry[0].dbc = buf_size - 1;
    cmdtbl->prdt_entry[0].i = 1;

    memset(&cmdtbl->cfis, 0, sizeof(FIS_H2D));
    memcpy(&cmdtbl->cfis, fis, sizeof(FIS_H2D));

    while (port->tfd & (ATA_STATUS_BSY | ATA_STATUS_DRQ)) {

    }

    port->ci = 1 << slot;

    while ((port->ci & (1 << slot))) {
        if (port->is & HBA_PxIS_TFES) {
            _ds->Println("AHCI: Task file error");
            return false;
        }
    }

    if (port->is & HBA_PxIS_TFES) {
        return false;
    }

    return true;
}

bool GenericAHCIController::cd_send_cmd(HBA_PORT* port, FIS_H2D* fis, void* buffer, uint32_t bsize, uint8_t* packet, size_t len)  {
    uint32_t slot = 0xFFFFFFFF;
    uint32_t timer = 100000;
    while (slot == 0xFFFFFFFF) {
        uint32_t slots = port->sact | port->ci;
        for (int i = 0; i < 32; i++) {
            if ((slots & (1 << i)) == 0) {
                slot = i;
                break;
            }
        }
        _ds->sleep(5);
        if (timer-- == 0) {
            return false;
        }
    }

    port->is = (uint32_t)-1;

    HBA_CMD* cmdheader = (HBA_CMD*)port->clb;
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(0xFFFFFFFF00000000 + cmdheader[slot].ctba);

    cmdheader[slot].cfl = sizeof(FIS_H2D) / sizeof(uint32_t);
    cmdheader[slot].w = (buffer && bsize > 0) ? 0 : 1;
    cmdheader[slot].prdtl = (buffer && bsize > 0) ? 1 : 0;
    cmdheader[slot].prdbc = 0;

    if (buffer && bsize > 0) {
        cmdtbl->prdt_entry[0].dba = (uint32_t)((uintptr_t)buffer & 0xFFFFFFFF);
        cmdtbl->prdt_entry[0].dbau = (uint32_t)(((uintptr_t)buffer >> 32) & 0xFFFFFFFF);
        cmdtbl->prdt_entry[0].dbc = bsize - 1;
        cmdtbl->prdt_entry[0].i = 1;
    }

    memset(&cmdtbl->cfis, 0, sizeof(FIS_H2D));
    memcpy(&cmdtbl->cfis, fis, sizeof(FIS_H2D));

    memcpy(((uint8_t*)&cmdtbl->cfis) + sizeof(FIS_H2D), packet, len);

    while (port->tfd & (ATA_STATUS_BSY | ATA_STATUS_DRQ)) {

    }

    port->ci = 1 << slot;

    while ((port->ci & (1 << slot))) {
        if (port->is & HBA_PxIS_TFES) {
            port->is = (uint32_t)-1;
            return false;
        }
    }

    if (port->is & HBA_PxIS_TFES) {
        return false;
    }

    return true;
}

/*
 * The OSDev Wiki didn't really explain
 * what AHCI is and how it works, so Ill
 * explain it instead.
 * 
 * AHCI is a standard for SATA Controllers
 * which replaces the old IDE Standard.
 * 
 * AHCI allows the OS to use a command like
 * system that allows for command queuing
 * and stuff for faster processing.
 * 
 * It also allows the user to hot plug the
 * drives and it uses MMIO like PCIe.
 * 
 * AHCI uses Host Bus Adapter (HBA) Memory
 * which exposes BAR5 in PCI called ABAR
 * which stands for AHCI Base Memory. The
 * ABAR has registers, which are CAP - 
 * Capabilities, GHC - Global Control,  
 * PI - Active Ports and Ports Array.
 * 
 * Each SATA port in AHCI has a Command
 * List Base which is a memory region 
 * where we put our commands.
 * 
 * The FIS Base is the memory region where
 * we put our Frame Information Structure
 * which is the struct the HBA and the 
 * drive use. There is FIS_REG_H2D and 
 * FIS_DMA_SETUP for HOST to DEVICE and
 * FIS_REG_D2H, FIS_PIO_SETUP and FIS_DEV_BITS
 * for DEVICE to HOST
 * 
 * So to initialize our AHCI we need to Enable
 * Interrupts, DMA and Memory Space Access in 
 * the PCI command register.
 * 
 * Then we must map BAR5 *WITHOUT* caching.
 * 
 * Then we should perform BIOS/OS handoff which
 * means that we check if the BIOS is done
 * handling the AHCI Controller so that we don't
 * corrupt the controller
 * 
 * After that, we should reset the controller and
 * register the IRQ handler as well as enabling
 * AHCI mode and interrupts in the global host 
 * control register.
 * 
 * Then we can read the capability registers and
 * check if 64-Bit DMA is Supported (optional).
 * 
 * Then for each port you must: *IN ORDER*
 *  - Stop the port
 *  - Allocate memory for the command list the FIS and the command tables
 *  - Memory map them (no caching)
 *  - Set the command list and the recieved FIS Address Registers
 *  - Setup command list entries to point to the command table.
 *  - Reset the port
 *  - Start command list processing with the port's command register.
 *  - Enable interrupts for the port
 *  - Read status to see if it connected to a drive.
 *  - Send `IDENTIFY ATA` command to connected drives. 
 *  - Get their sector size and count.
*/
void GenericAHCIController::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

    void* mem = _ds->malloc(sizeof(GenericAHCIFactory));
    if (!mem) {
        _ds->Println("Failed to Malloc for Generic AHCI Factory");
        _ds->Println(to_hstridng((uint64_t)mem));
    }

    /*
     * Don't Register the factory bc we don't want the DriverManager
     * to make more Generic AHCI Devices, since we are handling that.
    */
    GenericAHCIFactory* factory = new(mem) GenericAHCIFactory();

    uint16_t cmd;
    if (devKey.PCIe) {
        cmd = _ds->ConfigReadWorde(devKey.segment, devKey.bus, devKey.device, devKey.function, 0x04);
    } else {
        cmd = _ds->ConfigReadWord(devKey.bus, devKey.device, devKey.function, 0x04);
    }

    /*
     * Enable Memory Space, Bus Master and Interrupts
    */
    cmd |= (1 << 1);
    cmd |= (1 << 2);
    cmd &= ~(1 << 10);

    if (devKey.PCIe) {
        _ds->ConfigWriteWorde(devKey.segment, devKey.bus, devKey.device, devKey.function, 0x04, cmd);
    } else {
        _ds->ConfigWriteWord(devKey.bus, devKey.device, devKey.function, 0x04, cmd);
    }

    uint32_t bar5phys;
    if (devKey.PCIe) {
        bar5phys = _ds->ConfigReadDWorde(devKey.segment, devKey.bus, devKey.device, devKey.function, 0x24);
    } else {
        bar5phys = _ds->ConfigReadDWord(devKey.bus, devKey.device, devKey.function, 0x24);
    }
    uint64_t bar5 = 0xFFFFFFFF00000000 + bar5phys;
    hba = (HBA_MEM*)bar5;
    _ds->MapMemory((void*)bar5, (void*)bar5phys, false);
    _ds->Print("Bar 5: ");
    _ds->Println(to_hstridng(bar5));
    
    uint32_t cap = hba->cap;
    uint32_t ports = hba->pi;
    uint32_t version = hba->vs;
    uint8_t numPorts = cap & 0x1F;

    _ds->Print("AHCI Version: ");
    if (version == 0x10000) {
        _ds->Println("1.0");
    } else {
        _ds->Println(to_hstridng(version));
    }

    /*
     * We must request OS ownership
     * and set OSXS to say that the
     * os now owns the controller.
     * 
     * Then we wait for the BIOS to
     * finish.
    */
    volatile uint32_t* ghc = &hba->ghc;
    if (hba->cap2 & (1 << 31)) {
        _ds->Println("BIOS/OS handoff supported");
        if (*ghc & (1 << 30)) {
            *ghc |= (1 << 31);

            while (*ghc & (1 << 30)) {
                
            }
        }
    }
    
    /*
     * Enable AHCI Mode and interrupts
    */
    *ghc |= (1 << 31);
    *ghc |= (1 << 1);
    
    /*
     * Check info in Capabilities ptr.
    */
    bool supports64BitDMA = (cap & (1 << 31)) != 0;

    /*
     * I'd prefer to make my own code,
     * but this func from the OSDev 
     * wiki should just be for debugging.
     * 
     * We don't need this for now, bc it
     * takes up too much console space.
    */
    //probe_port(hba);

    for (uint8_t port = 0; port < numPorts; port++) {
        HBA_PORT* p = &hba->ports[port];

        stop_cmd(p);

        /*
         * We can skip empty ports to avoid
         * allocating unnecessary memory.
        */
        if (check_type(p) == AHCI_DEV_NULL) {
            continue;
        }

        /*
         * We need to stop the port by
         * clearing the ST bit and the FRE 
         * bit, then we can wait until the FR 
         * and CR bits are cleared.
         * 
         * This is because we need to alloc 
         * mem for the AHCI stuff
        */
        p->cmd &= ~HBA_PxCMD_ST;
        p->cmd &= ~HBA_PxCMD_FRE;

        while ((p->cmd & (HBA_PxCMD_FR | HBA_PxCMD_CR)) != 0) {
            
        }

        /*
         * We must allocate enough memory for the
         * Command tables, FISes and Command List
         * 
         * The Command List must be *1KB* aligned
         * The FIS (FB) must be *256Byte* aligned
         * The Command Tables are aligned by 128B
        */
        uintptr_t clb_phys = (uint64_t)_ds->RequestPage();
        uintptr_t page_end = clb_phys + 0x1000;

        clb_phys = (clb_phys + 1023) & ~((uintptr_t)1023);

        uintptr_t clb_virt = 0xFFFFFFFF00000000 + clb_phys;

        _ds->MapMemory((void*)clb_virt, (void*)clb_phys, false);
        memset((void*)clb_virt, 0, 1024);

        p->clb = (uint32_t)clb_phys;
        p->clbu = (uint32_t)(clb_phys >> 32);

        /*
         * Since we allocated a whole *4KB* page,
         * we can just find the end of the clb to
         * use for our FIS.
        */
        uintptr_t fb_phys = clb_phys + 1024;

        fb_phys = (fb_phys + 255) & ~((uintptr_t)255);

        uintptr_t fb_virt = 0xFFFFFFFF00000000 + fb_phys;

        memset((void*)fb_virt, 0, 256);

        p->fb = fb_phys;
        p->fbu = (fb_phys >> 32);

        /*
         * Now we get to the tricky part, we must
         * allocate enough memory for the Command
         * Tables.
         * 
         * I wanted maximum memory efficiency, so
         * I used the previous page from allocing
         * a page for the clb. Then I added an if
         * statement to check if there wasn't any
         * space left and to request more pages.
         * 
         * Also, make sure you map these as unca-
         * -cheable so that they won't be slow or
         * inaccurate.
         * 
         * Each Command Table is 256 Bytes long &
         * is aligned by 128 Bytes.
        */
        HBA_CMD* cmdheader = (HBA_CMD*)p->clb;
        uint64_t cmdPorts = ((cap >> 8) & 0x1F) + 1;

        uint64_t currPhys = fb_phys + 256;

        for (int i = 0; i < cmdPorts; i++) {
            currPhys = (currPhys + 127) & ~((uintptr_t)127);
            if ((currPhys + 256) > page_end) {
                currPhys = (uint64_t)_ds->RequestPage();
                page_end = currPhys + 0x1000;

                uint64_t currVirt = 0xFFFFFFFF00000000 + currPhys;

                _ds->MapMemory((void*)currVirt, (void*)currPhys, false);
            }

            uintptr_t currVirt = 0xFFFFFFFF00000000 + currPhys;

            memset((void*)currVirt, 0, 256);

            cmdheader[i].ctba = (uint32_t)currPhys;
            cmdheader[i].ctbau = (uint32_t)(currPhys >> 32);
            cmdheader[i].prdtl = 8;

            currPhys = currPhys + 256;
        }

        start_cmd(p);

        /*
         * Lets see if it's reset
        */
        uint32_t ssts = p->ssts;
        uint8_t det = ssts & 0x0F;
        uint8_t ipm = (ssts >> 8) & 0x0F;

        if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE) {
            _ds->Println("Failed to reset Port!!");
        }

        /*
         * Now we can enable Interrupts
        */
        p->ie = (1 << 30);

        /*
         * Pretty much the same as before
        */
        if (det == HBA_PORT_DET_PRESENT && ipm == HBA_PORT_IPM_ACTIVE) {
            //_ds->Println("Drive is Connected!!");
        } else {
            _ds->Println("No Drive/ Inactive Port");
            continue;
        }

        /*
         * We can finally send the IDENTIFY
         * command and get some info on the
         * drive.
        */
        if (check_type(p) == AHCI_DEV_SATA) {
            FIS_H2D fis;
            memset(&fis, 0, sizeof(FIS_H2D));
            fis.fis_type = FIS_TYPE_REG_H2D;
            fis.c = 1;
            fis.command = ATA_CMD_IDENTIFY;
            fis.device = 0;

            uint64_t buf_phys = (uint64_t)_ds->RequestPage();
            uint64_t buf_virt = 0xFFFFFFFF00000000 + buf_phys;

            _ds->MapMemory((void*)buf_virt, (void*)buf_phys, false);

            memset((void*)buf_virt, 0, 512);

            if (ahci_send_cmd(p, &fis, (void*)buf_phys, 512)) {
                ATA_IDENTIFY_DATA* id = (ATA_IDENTIFY_DATA*)buf_virt;
                
                uint32_t sectors = id->user_addressable_sectors_lo | (id->user_addressable_sectors_hi << 16);
                uint32_t sector_size = id->logic_sector_size_lo | (id->logic_sector_size_hi << 16);

                if (sector_size == 0) {
                    sector_size = 512;
                }

                portInfo[port] = id;
                
                BaseDriver* device = factory->CreateDevice();

                BaseDriver* dev = this;
                DeviceKey DevK;
                DevK.bars[0] = ((uint64_t)dev >> 32);
                DevK.bars[1] = ((uint64_t)dev & 0xFFFFFFFF);
                DevK.bars[2] = port;

                device->Init(ds, DevK);
                _ds->AddDriver(device);
            } else {
                _ds->Println("IDENTIFY ATA failed");
            }
        } else if (check_type(p) == AHCI_DEV_SATAPI) {
            uint8_t tur_packet[12] = {0x00};

            FIS_H2D cdfis{};
            memset(&cdfis, 0, sizeof(FIS_H2D));
            cdfis.fis_type = FIS_TYPE_REG_H2D;
            cdfis.c = 1;
            cdfis.command = ATA_CMD_PACKET;
            cdfis.device = 0;

            if (!cd_send_cmd(p, &cdfis, nullptr, 0, tur_packet, 12)) {
                _ds->Println("No CD present in drive");
            } else {
                //_ds->Println("CD detected! Safe to send IDENTIFY PACKET");
                FIS_H2D fis;
                memset(&fis, 0, sizeof(FIS_H2D));
                fis.fis_type = FIS_TYPE_REG_H2D;
                fis.c = 1;
                fis.command = ATA_CMD_IDENTIFY;
                fis.device = 0;

                uint64_t buf_phys = (uint64_t)_ds->RequestPage();
                uint64_t buf_virt = 0xFFFFFFFF00000000 + buf_phys;

                _ds->MapMemory((void*)buf_virt, (void*)buf_phys, false);

                memset((void*)buf_virt, 0, 512);

                if (ahci_send_cmd(p, &fis, (void*)buf_phys, 512)) {
                    ATA_IDENTIFY_DATA* id = (ATA_IDENTIFY_DATA*)buf_virt;
                    
                    uint32_t sectors = id->user_addressable_sectors_lo | (id->user_addressable_sectors_hi << 16);
                    uint32_t sector_size = id->logic_sector_size_lo | (id->logic_sector_size_hi << 16);

                    if (sector_size == 0) {
                        sector_size = 512;
                    }

                    portInfo[port] = id;

                    BaseDriver* device = factory->CreateDevice();

                    BaseDriver* dev = this;
                    DeviceKey DevK;
                    DevK.bars[0] = ((uint64_t)dev >> 32);
                    DevK.bars[1] = ((uint64_t)dev & 0xFFFFFFFF);
                    DevK.bars[2] = port;

                    device->Init(ds, DevK);
                    _ds->AddDriver(device);
                } else {
                    _ds->Println("IDENTIFY ATA failed");
                }
            }
        }
    }

    _ds->Println("Generic AHCI Initialized!");
}

/*
 * Now we get to the fun stuff.
 * 
 * We can now read a sector by
 * issuing a ATA_CMD_READ_DMA_EX
 * command using a FIS.
 * 
 * Btw, most of the code in this
 * function is from the OSDev
 * Wiki.
 * 
 * First we should clear any pending
 * interrupts.
 * 
 * Then we can use our ahci_send_cmd
 * helper to read the sector.
*/
bool GenericAHCIController::ReadSector(uint8_t drive, uint64_t lba, void* buffer) {
    HBA_PORT* port = &hba->ports[drive];
    port->is = (uint32_t)-1;

    FIS_H2D fis{};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.c = 1;
    fis.command = ATA_CMD_READ_DMA_EXT;
    fis.device = 1 << 6;

    fis.lba0 = (uint8_t)(lba & 0xFF);
    fis.lba1 = (uint8_t)((lba >> 8) & 0xFF);
    fis.lba2 = (uint8_t)((lba >> 16) & 0xFF);
    fis.lba3 = (uint8_t)((lba >> 24) & 0xFF);
    fis.lba4 = (uint8_t)((lba >> 32) & 0xFF);
    fis.lba5 = (uint8_t)((lba >> 40) & 0xFF);

    fis.countl = 1;
    fis.counth = 0;

    bool success = ahci_send_cmd(port, &fis, buffer, 512, 1);
    if (!success) {
        _ds->Println("ReadSector: Failed");
        return false;
    }

    return true;
}

/*
 * The Write Sector function is
 * pretty much the same except
 * we need to issue an ATA CMD
 * WRITE DMA EXT command and we
 * need to set the write bit.
*/
bool GenericAHCIController::WriteSector(uint8_t drive, uint64_t lba, void* buffer) {
    HBA_PORT* port = &hba->ports[drive];
    port->is = (uint32_t)-1;

    FIS_H2D fis{};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.c = 1;
    fis.command = ATA_CMD_WRITE_DMA_EXT;
    fis.device = 1 << 6;

    fis.lba0 = (uint8_t)(lba & 0xFF);
    fis.lba1 = (uint8_t)((lba >> 8) & 0xFF);
    fis.lba2 = (uint8_t)((lba >> 16) & 0xFF);
    fis.lba3 = (uint8_t)((lba >> 24) & 0xFF);
    fis.lba4 = (uint8_t)((lba >> 32) & 0xFF);
    fis.lba5 = (uint8_t)((lba >> 40) & 0xFF);

    fis.countl = 1;
    fis.counth = 0;

    bool success = ahci_send_cmd(port, &fis, buffer, 512, 1, true);
    if (!success) {
        _ds->Println("WriteSector: Failed");
        return false;
    }

    return true;
}

uint64_t GenericAHCIController::SectorCount(uint8_t drive) const {
    uint32_t sectors = portInfo[drive]->user_addressable_sectors_lo | (portInfo[drive]->user_addressable_sectors_hi << 16);
    return sectors;
}

uint32_t GenericAHCIController::SectorSize(uint8_t drive) const {
    uint32_t sector_size = portInfo[drive]->logic_sector_size_lo | (portInfo[drive]->logic_sector_size_hi << 16);

    if (sector_size == 0) {
        sector_size = 512;
    }
    return sector_size;
}

void* GenericAHCIController::GetInternalBuffer(uint8_t drive) {
    return (void*)0x0;
}

uint8_t GenericAHCIController::GetClass() {
    return devKey.classCode;
}

uint8_t GenericAHCIController::GetSubClass() {
    return devKey.subclass;
}

uint8_t GenericAHCIController::GetProgIF() {
    return devKey.progIF;
}

const char* GenericAHCIController::name(uint8_t drive) const {
    uint16_t* modelWords = (uint16_t*)portInfo[drive]->ModelName;
    char model[41];
    for (int i = 0; i < 20; i++) {
        model[i*2]   = modelWords[i] >> 8;
        model[i*2+1] = modelWords[i] & 0xFF;
    }
    model[40] = '\0';
    return _ds->strdup(model);
}

/*
 * Make sure you return somthing
 * otherwise you will get weird
 * errors, like code that ends up
 * at 0x8 in mem.
*/
const char* GenericAHCIController::DriverName() const {
    return "Generic AHCI Driver";
}
