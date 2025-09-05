#pragma once
#include <cstdint>

#define ELF_NIDENT 16

#define ELFMAG0	0x7F // e_ident[EI_MAG0]
#define ELFMAG1	'E'  // e_ident[EI_MAG1]
#define ELFMAG2	'L'  // e_ident[EI_MAG2]
#define ELFMAG3	'F'  // e_ident[EI_MAG3]

#define ELFDATA2LSB	(1)  // Little Endian
#define ELFCLASS32	(1)  // 32-bit Architecture

#define EM_386		(3)  // x86 Machine Type
#define EV_CURRENT	(1)  // ELF Current Version

#define SHN_UNDEF	(0x00) // Undefined/Not Present
#define SHN_ABS    0xFFF1 // Absolute symbol

#define ELF32_ST_BIND(INFO)	((INFO) >> 4)
#define ELF32_ST_TYPE(INFO)	((INFO) & 0x0F)

#define ELF32_R_SYM(INFO)	((INFO) >> 8)
#define ELF32_R_TYPE(INFO)	((uint8_t)(INFO))

#define ELF_RELOC_ERR -1

#define DO_386_32(S, A)	((S) + (A))
#define DO_386_PC32(S, A, P)	((S) + (A) - (P))

/*
 * These structs are from the OSDev Wiki:
 * https://wiki.osdev.org/ELF_Tutorial
*/
enum Elf_Ident {
	EI_MAG0		= 0, // 0x7F
	EI_MAG1		= 1, // 'E'
	EI_MAG2		= 2, // 'L'
	EI_MAG3		= 3, // 'F'
	EI_CLASS	= 4, // Architecture (32/64)
	EI_DATA		= 5, // Byte Order
	EI_VERSION	= 6, // ELF Version
	EI_OSABI	= 7, // OS Specific
	EI_ABIVERSION	= 8, // OS Specific
	EI_PAD		= 9  // Padding
};

enum Elf_Type {
	ET_NONE		= 0, // Unkown Type
	ET_REL		= 1, // Relocatable File
	ET_EXEC		= 2  // Executable File
};

enum ShT_Types {
	SHT_NULL	= 0,   // Null section
	SHT_PROGBITS	= 1,   // Program information
	SHT_SYMTAB	= 2,   // Symbol table
	SHT_STRTAB	= 3,   // String table
	SHT_RELA	= 4,   // Relocation (w/ addend)
	SHT_NOBITS	= 8,   // Not present in file
	SHT_REL		= 9,   // Relocation (no addend)
};

enum ShT_Attributes {
	SHF_WRITE	= 0x01, // Writable section
	SHF_ALLOC	= 0x02  // Exists in memory
};

enum StT_Bindings {
	STB_LOCAL		= 0, // Local scope
	STB_GLOBAL		= 1, // Global scope
	STB_WEAK		= 2  // Weak, (ie. __attribute__((weak)))
};

enum StT_Types {
	STT_NOTYPE		= 0, // No type
	STT_OBJECT		= 1, // Variables, arrays, etc.
	STT_FUNC		= 2  // Methods or functions
};

enum RtT_Types {
	R_386_NONE		= 0, // No relocation
	R_386_32		= 1, // Symbol + Offset
	R_386_PC32		= 2  // Symbol + Offset - Section Offset
};

typedef struct {
	uint8_t	 e_ident[ELF_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} Elf32_Shdr;

typedef struct {
	uint32_t		st_name;
	uint32_t		st_value;
	uint32_t		st_size;
	uint8_t			st_info;
	uint8_t			st_other;
	uint16_t		st_shndx;
} Elf32_Sym;

typedef struct {
	uint32_t		r_offset;
	uint32_t		r_info;
} Elf32_Rel;

typedef struct {
	uint32_t		r_offset;
	uint32_t		r_info;
	int32_t			r_addend;
} Elf32_Rela;

typedef struct {
	uint32_t		p_type;
	uint32_t		p_offset;
	uint32_t		p_vaddr;
	uint32_t		p_paddr;
	uint32_t		p_filesz;
	uint32_t		p_memsz;
	uint32_t		p_flags;
	uint32_t		p_align;
} Elf32_Phdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

typedef struct {
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint64_t      e_entry;
    uint64_t      e_phoff;
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf64_Ehdr;

/*
 * Stuff from the OSDev Wiki, we don't want these.
*/
bool elf_check_file(Elf32_Ehdr *hdr);
bool elf_check_supported(Elf32_Ehdr *hdr);
void *elf_load_rel(Elf32_Ehdr *hdr);
void *elf_load_file(void *file);
Elf32_Shdr *elf_sheader(Elf32_Ehdr *hdr);
Elf32_Shdr *elf_section(Elf32_Ehdr *hdr, int idx);
char *elf_str_table(Elf32_Ehdr *hdr);
char *elf_lookup_string(Elf32_Ehdr *hdr, int offset);
int elf_get_symval(Elf32_Ehdr *hdr, int table, uint32_t idx);

int elf_load_stage1(Elf32_Ehdr *hdr);
int elf_load_stage2(Elf32_Ehdr *hdr);

int elf_do_reloc(Elf32_Ehdr *hdr, Elf32_Rel *rel, Elf32_Shdr *reltab);

void *load_segment_to_memory(void *mem, Elf64_Phdr *phdr, int elf_fd);

/*
 * Our own code
*/
Elf64_Ehdr* GetELFHeader(void* data);
bool ValidateEhdr(Elf64_Ehdr* hdr);