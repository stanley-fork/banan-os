#pragma once

namespace LibELF
{

	enum ELF_Ident
	{
		ELFMAG0			= 0x7F,
		ELFMAG1			= 'E',
		ELFMAG2			= 'L',
		ELFMAG3			= 'F',

		ELFCLASSNONE	= 0,
		ELFCLASS32		= 1,
		ELFCLASS64		= 2,

		ELFDATANONE		= 0,
		ELFDATA2LSB		= 1,
		ELFDATA2MSB		= 2,
	};

	enum ELF_EI
	{
		EI_MAG0			= 0,
		EI_MAG1			= 1,
		EI_MAG2			= 2,
		EI_MAG3			= 3,
		EI_CLASS		= 4,
		EI_DATA			= 5,
		EI_VERSION		= 6,
		EI_OSABI		= 7,
		EI_ABIVERSION	= 8,
		EI_NIDENT		= 16,
	};

	enum ELF_ET
	{
		ET_NONE		= 0,
		ET_REL		= 1,
		ET_EXEC		= 2,
		ET_DYN		= 3,
		ET_CORE		= 4,
		ET_LOOS		= 0xfe00,
		ET_HIOS		= 0xfeff,
		ET_LOPROC	= 0xff00,
		ET_HIPROC	= 0xffff,
	};

	enum ELF_EV
	{
		EV_NONE		= 0,
		EV_CURRENT	= 1,
	};

	enum ELF_SHT
	{
		SHT_NULL		= 0,
		SHT_PROGBITS	= 1,
		SHT_SYMTAB		= 2,
		SHT_STRTAB		= 3,
		SHT_RELA		= 4,
		SHT_NOBITS		= 8,
		SHT_REL			= 9,
		SHT_SHLIB		= 10,
		SHT_DYNSYM		= 11,
		SHT_LOOS		= 0x60000000,
		SHT_HIOS		= 0x6FFFFFFF,
		SHT_LOPROC		= 0x70000000,
		SHT_HIPROC		= 0x7FFFFFFF,
	};

	enum ELF_SHF
	{
		SHF_WRITE		= 0x1,
		SHF_ALLOC		= 0x2,
		SHF_EXECINSTR	= 0x4,
		SHF_MASKOS		= 0x0F000000,
		SHF_MASKPROC	= 0xF0000000,
	};

	enum ELF_SHN
	{
		SHN_UNDEF	= 0,
		SHN_LOPROC	= 0xFF00,
		SHN_HIPROC	= 0xFF1F,
		SHN_LOOS	= 0xFF20,
		SHN_HIOS	= 0xFF3F,
		SHN_ABS		= 0xFFF1,
		SHN_COMMON	= 0xFFF2,
	};

#define ELF_ST_BIND(i) ((i) >> 4)
	enum ELF_STB
	{
		STB_LOCAL	= 0,
		STB_GLOBAL	= 1,
		STB_WEAK	= 2,
		STB_LOOS	= 10,
		STB_HIOS	= 12,
		STB_LOPROC	= 13,
		STB_HIPROC	= 15,
	};

#define ELF_ST_TYPE(i) ((i) & 0xF)
	enum ELF_STT
	{
		STT_NOTYPE	= 0,
		STT_OBJECT	= 1,
		STT_FUNC	= 2,
		STT_SECTION	= 3,
		STT_FILE	= 4,
		STT_TLS		= 6,
		STT_LOOS	= 10,
		STT_HIOS	= 12,
		STT_LOPROC	= 13,
		STT_HIPROC	= 15,
	};

	enum ELF_PT
	{
		PT_NULL		= 0,
		PT_LOAD		= 1,
		PT_DYNAMIC	= 2,
		PT_INTERP	= 3,
		PT_NOTE		= 4,
		PT_SHLIB	= 5,
		PT_PHDR		= 6,
		PT_TLS		= 7,
		PT_LOOS		= 0x60000000,
		PT_GNU_EH_FRAME = 0x6474E550,
		PT_GNU_STACK    = 0x6474E551,
		PT_GNU_RELRO    = 0x6474E552,
		PT_HIOS		= 0x6FFFFFFF,
		PT_LOPROC	= 0x70000000,
		PT_HIPROC	= 0x7FFFFFFF,
	};

	enum ELF_PF
	{
		PF_X		= 0x1,
		PF_W		= 0x2,
		PF_R		= 0x4,
		PF_MASKOS	= 0x00FF0000,
		PF_MASKPROC	= 0xFF000000,
	};

	enum ELF_DT
	{
		DT_NULL			= 0,
		DT_NEEDED		= 1,
		DT_PLTRELSZ		= 2,
		DT_PLTGOT		= 3,
		DT_HASH			= 4,
		DT_STRTAB		= 5,
		DT_SYMTAB		= 6,
		DT_RELA			= 7,
		DT_RELASZ		= 8,
		DT_RELAENT		= 9,
		DT_STRSZ		= 10,
		DT_SYMENT		= 11,
		DT_INIT			= 12,
		DT_FINI			= 13,
		DT_SONAME		= 14,
		DT_RPATH		= 15,
		DT_SYMBOLIC		= 16,
		DT_REL			= 17,
		DT_RELSZ		= 18,
		DT_RELENT		= 19,
		DT_PLTREL		= 20,
		DT_DEBUG		= 21,
		DT_TEXTREL		= 22,
		DT_JMPREL		= 23,
		DT_BIND_NOW		= 24,
		DT_INIT_ARRAY	= 25,
		DT_FINI_ARRAY	= 26,
		DT_INIT_ARRAYSZ	= 27,
		DT_FINI_ARRAYSZ	= 28,
		DT_LOOS			= 0x60000000,
		DT_HIOS			= 0x6FFFFFFF,
		DT_LOPROC		= 0x70000000,
		DT_HIPROC		= 0x7FFFFFFF,
	};

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((i) & 0xFF)
	enum ELF_R_386
	{
		R_386_NONE     = 0,
		R_386_32       = 1,
		R_386_PC32     = 2,
		R_386_GOT32    = 3,
		R_386_PLT32    = 4,
		R_386_COPY     = 5,
		R_386_GLOB_DAT = 6,
		R_386_JMP_SLOT = 7,
		R_386_RELATIVE = 8,
		R_386_GOTOFF   = 9,
		R_386_GOTPC    = 10,
		R_386_TLS_TPOFF		= 14,
		R_386_TLS_IE		= 15,
		R_386_TLS_GOTIE		= 16,
		R_386_TLS_LE		= 17,
		R_386_TLS_GD		= 18,
		R_386_TLS_LDM		= 19,
		R_386_TLS_GD_32		= 24,
		R_386_TLS_GD_PUSH	= 25,
		R_386_TLS_GD_CALL	= 26,
		R_386_TLS_GD_POP	= 27,
		R_386_TLS_LDM_32	= 28,
		R_386_TLS_LDM_PUSH	= 29,
		R_386_TLS_LDM_CALL	= 30,
		R_386_TLS_LDM_POP	= 31,
		R_386_TLS_LDO_32	= 32,
		R_386_TLS_IE_32		= 33,
		R_386_TLS_LE_32		= 34,
		R_386_TLS_DTPMOD32	= 35,
		R_386_TLS_DTPOFF32	= 36,
		R_386_TLS_TPOFF32	= 37,
	};

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xFFFFFFFF)
	enum ELF_R_X86_64
	{
		R_X86_64_NONE            = 0,
		R_X86_64_64              = 1,
		R_X86_64_PC32            = 2,
		R_X86_64_GOT32           = 3,
		R_X86_64_PLT32           = 4,
		R_X86_64_COPY            = 5,
		R_X86_64_GLOB_DAT        = 6,
		R_X86_64_JUMP_SLOT       = 7,
		R_X86_64_RELATIVE        = 8,
		R_X86_64_GOTPCREL        = 9,
		R_X86_64_32              = 10,
		R_X86_64_32S             = 11,
		R_X86_64_16              = 12,
		R_X86_64_PC16            = 13,
		R_X86_64_8               = 14,
		R_X86_64_PC8             = 15,
		R_X86_64_DTPMOD64        = 16,
		R_X86_64_DTPOFF64        = 17,
		R_X86_64_TPOFF64         = 18,
		R_X86_64_TLSGD           = 19,
		R_X86_64_TLSLD           = 20,
		R_X86_64_DTPOFF32        = 21,
		R_X86_64_GOTTPOFF        = 22,
		R_X86_64_TPOFF32         = 23,
		R_X86_64_PC64            = 24,
		R_X86_64_GOTOFF64        = 25,
		R_X86_64_GOTPC32         = 26,
		R_X86_64_SIZE32          = 32,
		R_X86_64_SIZE64          = 33,
		R_X86_64_GOTPC32_TLSDESC = 34,
		R_X86_64_TLSDESC_CALL    = 35,
		R_X86_64_TLSDESC         = 36,
		R_X86_64_IRELATIVE       = 37,
	};

}
