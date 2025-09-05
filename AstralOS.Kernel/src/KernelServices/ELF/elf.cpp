#include "elf.h"
#include "../KernelServices.h"

Elf64_Ehdr* GetELFHeader(void* data) {
    Elf64_Ehdr *hdr = (Elf64_Ehdr*)data;
}

bool ValidateEhdr(Elf64_Ehdr* hdr) {
    if (hdr->e_ident[0] != 0x7F || hdr->e_ident[1] != 'E' || hdr->e_ident[2] != 'L' || hdr->e_ident[3] != 'F') {
        ks->basicConsole.Println("Invalid ELF magic.");
        return false;
    }

    if (hdr->e_type != ET_EXEC) {
        ks->basicConsole.Println("Not an executable ELF.");
        return false;
    }

    return true;
}