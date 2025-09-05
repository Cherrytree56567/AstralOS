#include "elf.h"
#include "../KernelServices.h"
#include <sys/types.h>

/*
bool elf_check_file(Elf32_Ehdr *hdr) {
	if(!hdr) return false;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
		ks->basicConsole.Println("ELF Header EI_MAG0 incorrect.");
		return false;
	}
	if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
		ks->basicConsole.Println("ELF Header EI_MAG1 incorrect.");
		return false;
	}
	if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
		ks->basicConsole.Println("ELF Header EI_MAG2 incorrect.");
		return false;
	}
	if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
		ks->basicConsole.Println("ELF Header EI_MAG3 incorrect.");
		return false;
	}
	return true;
}

bool elf_check_supported(Elf32_Ehdr *hdr) {
	if(!elf_check_file(hdr)) {
		ks->basicConsole.Println("Invalid ELF File.");
		return false;
	}
	if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		ks->basicConsole.Println("Unsupported ELF File Class.");
		return false;
	}
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		ks->basicConsole.Println("Unsupported ELF File byte order.");
		return false;
	}
	if(hdr->e_machine != EM_386) {
		ks->basicConsole.Println("Unsupported ELF File target.");
		return false;
	}
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		ks->basicConsole.Println("Unsupported ELF File version.");
		return false;
	}
	if(hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
		ks->basicConsole.Println("Unsupported ELF File type.");
		return false;
	}
	return true;
}

void *elf_load_rel(Elf32_Ehdr *hdr) {
	int result;
	result = elf_load_stage1(hdr);
	if(result == ELF_RELOC_ERR) {
		ks->basicConsole.Println("Unable to load ELF file.");
		return NULL;
	}
	result = elf_load_stage2(hdr);
	if(result == ELF_RELOC_ERR) {
		ks->basicConsole.Println("Unable to load ELF file.");
		return NULL;
	}
	// TODO : Parse the program header (if present)
	return (void *)hdr->e_entry;
}

void *elf_load_file(void *file) {
	Elf32_Ehdr *hdr = (Elf32_Ehdr *)file;
	if(!elf_check_supported(hdr)) {
		ks->basicConsole.Println("ELF File cannot be loaded.");
		return;
	}
	switch(hdr->e_type) {
		case ET_EXEC:
			// TODO : Implement
			return NULL;
		case ET_REL:
			return elf_load_rel(hdr);
	}
	return NULL;
}

Elf32_Shdr *elf_sheader(Elf32_Ehdr *hdr) {
	return (Elf32_Shdr *)((int)hdr + hdr->e_shoff);
}

Elf32_Shdr *elf_section(Elf32_Ehdr *hdr, int idx) {
	return &elf_sheader(hdr)[idx];
}

char *elf_str_table(Elf32_Ehdr *hdr) {
	if(hdr->e_shstrndx == SHN_UNDEF) return NULL;
	return (char *)hdr + elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}

char *elf_lookup_string(Elf32_Ehdr *hdr, int offset) {
	char *strtab = elf_str_table(hdr);
	if(strtab == NULL) return NULL;
	return strtab + offset;
}

int elf_get_symval(Elf32_Ehdr *hdr, int table, uint32_t idx) {
	if(table == SHN_UNDEF || idx == SHN_UNDEF) return 0;
	Elf32_Shdr *symtab = elf_section(hdr, table);

	uint32_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
	if(idx >= symtab_entries) {
		ks->basicConsole.Print("Symbol Index out of Range (");
        ks->basicConsole.Print(to_hstring((uint64_t)table)); 
        ks->basicConsole.Print(":");
        ks->basicConsole.Println(to_hstring(idx));
		return ELF_RELOC_ERR;
	}

	int symaddr = (int)hdr + symtab->sh_offset;
	Elf32_Sym *symbol = &((Elf32_Sym *)symaddr)[idx];

    if (symbol->st_shndx == SHN_UNDEF) {
		// External symbol, lookup value
		Elf32_Shdr *strtab = elf_section(hdr, symtab->sh_link);
		const char *name = (const char *)hdr + strtab->sh_offset + symbol->st_name;

		extern void *elf_lookup_symbol(const char *name);
		void *target = elf_lookup_symbol(name);

		if(target == NULL) {
			// Extern symbol not found
			if(ELF32_ST_BIND(symbol->st_info) & STB_WEAK) {
				// Weak symbol initialized as 0
				return 0;
			} else {
				ks->basicConsole.Print("Undefined External Symbol : ");
                ks->basicConsole.Println(name);
				return ELF_RELOC_ERR;
			}
		} else {
			return (int)target;
		}
    } else if(symbol->st_shndx == SHN_ABS) {
		// Absolute symbol
		return symbol->st_value;
	} else {
		// Internally defined symbol
		Elf32_Shdr *target = elf_section(hdr, symbol->st_shndx);
		return (int)hdr + symbol->st_value + target->sh_offset;
	}
}

int elf_load_stage1(Elf32_Ehdr *hdr) {
	Elf32_Shdr *shdr = elf_sheader(hdr);

	unsigned int i;
	// Iterate over section headers
	for(i = 0; i < hdr->e_shnum; i++) {
		Elf32_Shdr *section = &shdr[i];

		// If the section isn't present in the file
		if(section->sh_type == SHT_NOBITS) {
			// Skip if it the section is empty
			if(!section->sh_size) continue;
			// If the section should appear in memory
			if(section->sh_flags & SHF_ALLOC) {
				// Allocate and zero some memory
				void *mem = malloc(section->sh_size);
				memset(mem, 0, section->sh_size);

				// Assign the memory offset to the section offset
				section->sh_offset = (int)mem - (int)hdr;
				ks->basicConsole.Print("Allocated memory for a section (");
                ks->basicConsole.Print(to_hstring(section->sh_size));
                ks->basicConsole.Println(").");
			}
		}
	}
	return 0;
}

int elf_load_stage2(Elf32_Ehdr *hdr) {
	Elf32_Shdr *shdr = elf_sheader(hdr);

	unsigned int i, idx;
	// Iterate over section headers
	for (i = 0; i < hdr->e_shnum; i++) {
		Elf32_Shdr *section = &shdr[i];
		
		// If this is a relocation section
		if (section->sh_type == SHT_REL) {
			// Process each entry in the table
			for (idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
				Elf32_Rel *reltab = &((Elf32_Rel *)((int)hdr + section->sh_offset))[idx];
				int result = elf_do_reloc(hdr, reltab, section);
				// On error, display a message and return
				if (result == ELF_RELOC_ERR) {
					ks->basicConsole.Println("Failed to relocate symbol.");
					return ELF_RELOC_ERR;
				}
			}
		}
	}
	return 0;
}

int elf_do_reloc(Elf32_Ehdr *hdr, Elf32_Rel *rel, Elf32_Shdr *reltab) {
	Elf32_Shdr *target = elf_section(hdr, reltab->sh_info);

	int addr = (int)hdr + target->sh_offset;
	int *ref = (int *)(addr + rel->r_offset);

    // Symbol value
	int symval = 0;
	if(ELF32_R_SYM(rel->r_info) != SHN_UNDEF) {
		symval = elf_get_symval(hdr, reltab->sh_link, ELF32_R_SYM(rel->r_info));
		if(symval == ELF_RELOC_ERR) return ELF_RELOC_ERR;
	}

    // Relocate based on type
	switch(ELF32_R_TYPE(rel->r_info)) {
		case R_386_NONE:
			// No relocation
			break;
		case R_386_32:
			// Symbol + Offset
			*ref = DO_386_32(symval, *ref);
			break;
		case R_386_PC32:
			// Symbol + Offset - Section Offset
			*ref = DO_386_PC32(symval, *ref, (int)ref);
			break;
		default:
			// Relocation type not supported, display error and return
			ks->basicConsole.Print("Unsupported Relocation Type (");
            ks->basicConsole.Print(to_hstring(ELF32_R_TYPE(rel->r_info)));
            ks->basicConsole.Println(").");
			return ELF_RELOC_ERR;
	}
	return symval;
}

void *load_segment_to_memory(void *mem, Elf64_Phdr *phdr, int elf_fd) {
    size_t mem_size = phdr->p_memsz;
    off_t mem_offset = phdr->p_offset;
    size_t file_size = phdr->p_filesz;
    void *vaddr = (void *)(phdr->p_vaddr);/* TODO: FIX THIS
    // mmap the memory region with the correct protections
    int prot = 0;
    if (phdr->p_flags & PF_R) prot |= PROT_READ;
    if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

    off_t page_offset = (uint64_t)vaddr % PAGE_SIZE;
    void *aligned_vaddr = (void *)((uint64_t)vaddr - page_offset);
    size_t aligned_size = mem_size + page_offset;

    mmap(aligned_vaddr, file_size + page_offset, prot, MAP_PRIVATE | MAP_ANONYMOUS, fd, mem_offset - page_offset);
    // technically we can just have vaddr as the first argument as mmap will 
    // automatically truncate to the start of the page
    if (mem_size > file_size) {
        void *page_break = ((uint64_t)vaddr + mem_offset + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        memset((uint64_t)vaddr + file_size, 0, (uint64_t)page_break - (uint64_t)vaddr - file_size);
        if (mem_size > page_break - (uint64_t)vaddr) {
            mmap(page_break, mem_size - (page_break - (uint64_t)vaddr), flags, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            memset(page_break, 0, mem_size - (page_break - (uint64_t)vaddr));
        }
    }
		* /
}
*/