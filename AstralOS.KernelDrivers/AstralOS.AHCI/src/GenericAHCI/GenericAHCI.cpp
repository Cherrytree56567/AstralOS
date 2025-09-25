#include "GenericAHCI.h"
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

bool GenericAHCI::Supports(const DeviceKey& devKey) {
    if (devKey.classCode == 0x01 && devKey.subclass == 0x6 && devKey.progIF == 0x1) {
        return true;
    }
    return false;
}

BlockDevice* GenericAHCI::CreateDevice() {
    void* mem = g_ds->malloc(sizeof(GenericAHCIDevice));
    if (!mem) {
        g_ds->Println("Failed to Malloc for Generic AHCI Device");
        return nullptr;
    }

    GenericAHCIDevice* device = new(mem) GenericAHCIDevice();
    return device;
}

void* memset(void* dest, uint8_t value, size_t num) {
    uint8_t* ptr = (uint8_t*)dest;
    for (size_t i = 0; i < num; i++) {
        ptr[i] = value;
    }
    return dest;
}

void GenericAHCIDevice::probe_port(HBA_MEM *abar) {
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

int GenericAHCIDevice::check_type(HBA_PORT *port) {
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

void GenericAHCIDevice::port_rebase(HBA_PORT *port, int portno) {
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

void GenericAHCIDevice::start_cmd(HBA_PORT *port) {
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

void GenericAHCIDevice::stop_cmd(HBA_PORT *port) {
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
 * Then for each port you must:
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
void GenericAHCIDevice::Init(DriverServices& ds, DeviceKey& dKey) {
    _ds = &ds;
    devKey = dKey;

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
    HBA_MEM* hba = (HBA_MEM*)bar5;
    _ds->MapMemory((void*)bar5, (void*)bar5phys, false);
    _ds->Print("Bar 5: ");
    _ds->Println(to_hstridng(bar5));
    
    uint32_t cap = hba->cap;
    uint32_t ports = hba->pi;
    uint32_t version = hba->vs;

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
     * To reset our controller we must
     * tell it to stop and wait for it
     * to stop.
     * 
     * Now we can tell it to reset and
     * then... done
    */
    *ghc &= ~(1 << 0);

    while (*ghc & (1 << 0)) {

    }

    *ghc |= (1 << 0);
    
    /*
     * Enable AHCI Mode and interrupts
    */
    *ghc |= (1 << 31);
    *ghc |= (1 << 1);
    
    /*
     * Check info in Capabilities ptr.
    */
    bool supports64BitDMA = (cap & (1 << 31)) != 0;
    uint8_t numPorts = cap & 0x1F;
    
    _ds->Print("64-bit DMA support: ");
    _ds->Println(supports64BitDMA ? "yes" : "no");
    _ds->Print("Number of ports: ");
    _ds->Println(to_hstridng(numPorts));

    for (uint8_t port = 0; port < numPorts; port++) {
        HBA_PORT* p = &hba->ports[port];

        uintptr_t clb_phys = (uint64_t)_ds->RequestPage();
        uintptr_t clb_virt = 0xFFFFFFFF00000000 + clb_phys;
        _ds->MapMemory((void*)clb_virt, (void*)clb_phys, false);
        clb_virt = (clb_virt + 1023) & ~((uintptr_t)1023);
        memset((void*)clb_virt, 0, 1024);

        p->clb = clb_phys;
        p->clbu = 0;

        _ds->Println("Allocated 1KB for Command List");
    }

    _ds->Println("Generic AHCI Initialized!");
}

bool GenericAHCIDevice::ReadSector(uint64_t lba, void* buffer) {
    return true;
}

bool GenericAHCIDevice::WriteSector(uint64_t lba, void* buffer) {
    return true;
}

uint64_t GenericAHCIDevice::SectorCount() const {
    return 0x0;
}

uint32_t GenericAHCIDevice::SectorSize() const {
    return 0x0;
}

void* GenericAHCIDevice::GetInternalBuffer() {
    return (void*)0x0;
}

uint8_t GenericAHCIDevice::GetClass() {
    return 0x0;
}

uint8_t GenericAHCIDevice::GetSubClass() {
    return 0x0;
}

uint8_t GenericAHCIDevice::GetProgIF() {
    return 0x0;
}

const char* GenericAHCIDevice::name() const {
    return "";
}

/*
 * Make sure you return somthing
 * otherwise you will get weird
 * errors, like code that ends up
 * at 0x8 in mem.
*/
const char* GenericAHCIDevice::DriverName() const {
    return "";
}
