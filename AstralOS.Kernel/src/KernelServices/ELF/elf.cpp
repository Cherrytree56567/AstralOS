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
        /*
         * WHY SOOO MANY VARIABLES???
        */
        uint64_t strtab, symtab, strsz, syment, rela, relasz, relaent, rel, relsz, relent, jmprel, pltrelsz, pltgot, init_array, init_arraysz = 0;
        int64_t pltrel = 0;
        while (dynamic->d_tag != DT_NULL) {
            if (dynamic->d_tag == DT_NULL) {
                return;
            }
            switch(dynamic->d_tag) {
                case DT_NULL:
                    break;

                case DT_PLTRELSZ:
                    pltrelsz = dynamic->d_un.d_val;
                    break;

                case DT_PLTGOT:
                    pltgot = dynamic->d_un.d_ptr;
                    break;

                case DT_STRTAB:
                    strtab = dynamic->d_un.d_ptr;
                    break;

                case DT_SYMTAB:
                    symtab = dynamic->d_un.d_ptr;
                    break;

                case DT_RELA:
                    rela = dynamic->d_un.d_ptr;
                    break;

                case DT_RELASZ:
                    relasz = dynamic->d_un.d_val;
                    break;

                case DT_RELAENT:
                    relaent = dynamic->d_un.d_val;
                    break;

                case DT_STRSZ:
                    strsz = dynamic->d_un.d_val;
                    break;

                case DT_SYMENT:
                    syment = dynamic->d_un.d_val;
                    break;

                case DT_REL:
                    rel = dynamic->d_un.d_ptr;
                    break;

                case DT_RELSZ:
                    relsz = dynamic->d_un.d_val;
                    break;

                case DT_RELENT:
                    relent = dynamic->d_un.d_val;
                    break;

                case DT_PLTREL:
                    pltrel = dynamic->d_un.d_val;
                    break;

                case DT_JMPREL:
                    jmprel = dynamic->d_un.d_ptr;
                    break;

                case DT_INIT_ARRAY:
                    init_array = dynamic->d_un.d_ptr;
                    break;

                case DT_INIT_ARRAYSZ:
                    init_arraysz = dynamic->d_un.d_val;
                    break;

                default:
                    ks->basicConsole.Print(((String)"ELF: Unhandled DT entry: " + to_string(dynamic->d_tag)).c_str());
                    break;
            }
            dynamic++;
        }


    }

    return (uint64_t)(base + hdr->e_entry);
}

void apply_relocations(uint8_t* base, uint64_t rela_vaddr, uint64_t relasz, uint64_t relaent, uint64_t symtab_vaddr, uint64_t strtab_vaddr, 
    uint64_t syment, void* (*symbol_resolver)(const char*)) {

}