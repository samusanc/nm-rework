/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   x86_utlis.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/02 02:01:55 by samusanc          #+#    #+#             */
/*   Updated: 2025/05/07 19:28:28 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_nm.h"

/*
	Mirror of the fix in src/x64/x64_utils.c's get_symbol_type_x64() --
	see that file for the full explanation. Short version: `st_shndx`
	comes straight from the (untrusted) file and was used to index
	`shdrs[]` with no bounds check; a corrupted/out-of-range value read
	past the real section table. `ehdr` is also now genuinely used
	(previously it was an unused parameter with a dead `(void)ehdr;`
	after the function's last reachable `return`).

	This project has to "handle x86_32 and x86_64 binaries" per the
	subject, and this file is the x86_32 half of the ELF parser -- every
	fix applied to the x64 path had to be mirrored here, since a 32-bit
	binary is just as capable of carrying a corrupted st_shndx as a
	64-bit one. Verified against a real i386 binary built with
	`gcc-multilib` (exe, .o and .so), all three byte-identical to real
	`nm`'s output, plus a stripped 32-bit binary confirmed to print
	"no symbols" instead of crashing.
*/
char	get_symbol_type_x86(Elf32_Sym *sym, Elf32_Shdr *shdrs, Elf32_Ehdr *ehdr)
{
	if (!sym || !shdrs || !ehdr)
		return 0;

	unsigned bind = ELF32_ST_BIND(sym->st_info);
    unsigned type = ELF32_ST_TYPE(sym->st_info);

    if (sym->st_shndx == SHN_UNDEF)
        return bind == STB_WEAK ? 'w' : 'U';
    if (sym->st_shndx == SHN_ABS)
        return bind == STB_LOCAL ? 'a' : 'A';
    if (sym->st_shndx == SHN_COMMON)
        return bind == STB_LOCAL ? 'c' : 'C';
    if (sym->st_shndx >= ehdr->e_shnum)
        return '?';

    Elf32_Shdr *sec = &shdrs[sym->st_shndx];

    if (type == STT_SECTION)
    {
        char c;
        if (sec->sh_type == SHT_NOBITS && (sec->sh_flags & SHF_WRITE))
            c = 'B';
        else if (sec->sh_flags & SHF_EXECINSTR)
            c = 'T';
        else if (sec->sh_flags & SHF_WRITE)
            c = 'D';
        else if (sec->sh_flags & SHF_ALLOC)
            c = 'R';
        else
            return 'N';

        if (bind == STB_LOCAL)
            c = ft_tolower(c);
        return c;
    }

    if (bind == STB_WEAK)
        return sym->st_shndx == SHN_UNDEF ? 'w' : 'W';

    {
        char c = '?';
        if (sec->sh_type == SHT_NOBITS && (sec->sh_flags & SHF_WRITE))
            c = 'B';
        else if (sec->sh_flags & SHF_EXECINSTR)
            c = 'T';
        else if (sec->sh_flags & SHF_WRITE)
            c = 'D';
        else if (sec->sh_flags & SHF_ALLOC)
            c = 'R';

        if (bind == STB_LOCAL)
            c = ft_tolower(c);
        return c;
    }
}

/*
	Mirror of range_in_bounds() in src/x64/x64_utils.c -- same overflow
	reasoning applies verbatim, just on the 32-bit Elf32_* types. See
	that file for the full explanation of why the addition-based check
	(`offset + size > file_size`) is unsafe and this subtraction-based
	version isn't.
*/
static int	range_in_bounds(size_t off, size_t size, size_t file_size)
{
	if (off > file_size)
		return 0;
	if (size > file_size - off)
		return 0;
	return 1;
}

/*
	Mirror of process_elf64() in src/x64/x64_utils.c -- same three bugs
	fixed here for the exact same reasons (NULL symtab_hdr/strtab_hdr on
	stripped binaries, overflow-prone bounds checks, unbounded string
	lookups). See that file for the full write-up; nothing here is
	x86-specific beyond the struct/macro names (Elf32_* instead of
	Elf64_*, ELF32_ST_TYPE instead of ELF64_ST_TYPE).
*/
int	process_elf32(void *mapped, t_list *output, size_t file_size, char *file)
{
	if (!mapped || !output)
		return 0;
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)mapped;

	if (ehdr->e_shnum == 0)
		return 0;
	if (!range_in_bounds(ehdr->e_shoff, (size_t)ehdr->e_shnum * sizeof(Elf32_Shdr), file_size))
		return error(file, "file format not recognized", 0);

	Elf32_Shdr *shdr = (Elf32_Shdr *)((char *)mapped + ehdr->e_shoff);

	if (ehdr->e_shstrndx >= ehdr->e_shnum)
		return error(file, "file format not recognized", 0);
	if (!range_in_bounds(shdr[ehdr->e_shstrndx].sh_offset, shdr[ehdr->e_shstrndx].sh_size, file_size))
		return error(file, "file format not recognized", 0);

	size_t shstrtab_size = shdr[ehdr->e_shstrndx].sh_size;
	const char *shstrtab = (char*)mapped + shdr[ehdr->e_shstrndx].sh_offset;
	Elf32_Shdr *symtab_hdr = NULL;
	Elf32_Shdr *strtab_hdr = NULL;

	for (int i = 0; i < ehdr->e_shnum; i++) {
		const char *name = safe_str(shstrtab, shdr[i].sh_name, shstrtab_size);
		if (shdr[i].sh_type == SHT_SYMTAB)
			symtab_hdr = &shdr[i];
		else if (shdr[i].sh_type == SHT_STRTAB && name && !(ft_strcmp(name, ".strtab")))
			strtab_hdr = &shdr[i];
	}

	/* Missing .symtab/.strtab -> normal "no symbols" case (stripped
	   32-bit binary), not a fatal error. See x64_utils.c for the
	   detailed rationale; this is the exact same fix, mirrored. */
	if (!symtab_hdr || !strtab_hdr)
		return 0;
	if (!range_in_bounds(symtab_hdr->sh_offset, symtab_hdr->sh_size, file_size))
		return error(file, "file format not recognized", 0);
	if (!range_in_bounds(strtab_hdr->sh_offset, strtab_hdr->sh_size, file_size))
		return error(file, "file format not recognized", 0);

	Elf32_Sym *symbols = (Elf32_Sym *)((char *)mapped + symtab_hdr->sh_offset);
	size_t num_symbols = symtab_hdr->sh_size / sizeof(Elf32_Sym);
	char *strtab = (char *)mapped + strtab_hdr->sh_offset;
	size_t strtab_size = strtab_hdr->sh_size;

	for (size_t i = 0; i < num_symbols; i++) {
		Elf32_Sym sym = symbols[i];
		unsigned char st_type = ELF32_ST_TYPE(sym.st_info);

		const char *name;
		if (st_type == STT_SECTION) {
			if (sym.st_shndx >= ehdr->e_shnum)
				continue;
			name = safe_str(shstrtab, shdr[sym.st_shndx].sh_name, shstrtab_size);
		} else {
			if (sym.st_name == 0)
				continue;
			name = safe_str(strtab, sym.st_name, strtab_size);
		}
		if (!name)
			continue;

		char type_char = get_symbol_type_x86(&sym, shdr, ehdr);
		t_header *header = malloc(sizeof(*header));
		if (!header)
			return error("fatal", "malloc allocation failed", 0);
		ft_bzero(header, sizeof(*header));
		header->addr      = sym.st_value;
		header->type_char = type_char;
		header->name      = ft_strdup(name);
		if (!header->name)
		{
			free(header);
			return error("fatal", "malloc allocation failed", 0);
		}
		list_push_b(output, node(header, free_header));
	}

	return 0;
}
