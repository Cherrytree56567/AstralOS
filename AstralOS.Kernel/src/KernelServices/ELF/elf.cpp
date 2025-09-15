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

uint64_t LoadElf(Elf64_Ehdr* hdr) {
    Elf64_Phdr* phdrs = (Elf64_Phdr*)((uint8_t*)hdr + hdr->e_phoff);

    size_t total_size = 0;
    size_t max_align = 0;

    for (int i = 0; i < hdr->e_phnum; i++) {
        Elf64_Phdr* ph = &phdrs[i];

        if (ph->p_type != PT_LOAD) {
            continue;
        }

        size_t end = ph->p_vaddr + ph->p_memsz;

        if (end > total_size) {
            total_size = end;
        }

        if (ph->p_align > max_align) {
            max_align = ph->p_align;
        }
    }

    size_t total = total_size + max_align;
    uint8_t* raw = (uint8_t*)malloc(total);
    uint8_t* base = (uint8_t*)(((uintptr_t)raw + (max_align - 1)) & ~(max_align - 1));
    if (!base) {
        return 0x0;
    }

    Elf64_Dyn* dynamic = nullptr;
    for (int i = 0; i < hdr->e_phnum; i++) {
        Elf64_Phdr* ph = &phdrs[i];
        if (ph->p_type != PT_LOAD) {
            continue;
        }

        if (ph->p_type == PT_DYNAMIC) {
            dynamic = (Elf64_Dyn*)((uint8_t*)hdr + ph->p_offset);
            break;
        }

        uint8_t* dest = base + ph->p_vaddr;
        uint8_t* src  = (uint8_t*)hdr + ph->p_offset;

        memcpy(dest, src, ph->p_filesz);
        if (ph->p_memsz > ph->p_filesz) {
            memset(dest + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz); 
        }
    }

    if (dynamic) {
        while (dynamic->d_tag != DT_NULL) {
            switch(dyn->d_tag) {
            }
        }
    }

    return (uint64_t)(base + hdr->e_entry);
}