#include "elf.h"
#include "../KernelServices.h"

Elf64_Ehdr* GetELFHeader(void* data) {
    Elf64_Ehdr *hdr = (Elf64_Ehdr*)data;
    return hdr;
}

bool ValidateEhdr(Elf64_Ehdr* hdr) {
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 || hdr->e_ident[EI_MAG1] != ELFMAG1 ||
        hdr->e_ident[EI_MAG2] != ELFMAG2 || hdr->e_ident[EI_MAG3] != ELFMAG3) {
        ks->basicConsole.Println("ELF: bad magic");
        return NULL;
    }
    if (hdr->e_ident[EI_CLASS] != ELF64) {
        ks->basicConsole.Println("ELF: not 64-bit");
        return NULL;
    }
    if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
        ks->basicConsole.Println("ELF: not ET_EXEC/ET_DYN (unsupported)");
        return NULL;
    }

    return true;
}

Elf64_Phdr* GetLoadablePhdr(Elf64_Ehdr* hdr) {

}