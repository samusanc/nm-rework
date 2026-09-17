/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   x64_utils.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/02 02:01:38 by samusanc          #+#    #+#             */
/*   Updated: 2025/05/07 19:00:49 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_nm.h"

char	get_symbol_type_x64(Elf64_Sym *sym, Elf64_Shdr *shdrs, Elf64_Ehdr *ehdr)
{
	if (!sym || !shdrs || !ehdr)
		return 0;

	unsigned bind = ELF64_ST_BIND(sym->st_info);
    unsigned type = ELF64_ST_TYPE(sym->st_info);

    if (sym->st_shndx == SHN_UNDEF)
        return bind == STB_WEAK ? 'w' : 'U';
    if (sym->st_shndx == SHN_ABS)
        return bind == STB_LOCAL ? 'a' : 'A';
    if (sym->st_shndx == SHN_COMMON)
        return bind == STB_LOCAL ? 'c' : 'C';

    Elf64_Shdr *sec = &shdrs[sym->st_shndx];

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
	(void)ehdr;
	return 0;
}

int	process_elf64(void *mapped, t_list *output, size_t file_size, char *file)
{
	if (!mapped || !output)
		return 0;
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)mapped;

	if (ehdr->e_shoff + (ehdr->e_shnum * sizeof(Elf64_Shdr)) > file_size)
	    return error(file, "file format not recognized", 0);

	Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)mapped + ehdr->e_shoff);

	if (ehdr->e_shstrndx >= ehdr->e_shnum)
	    return error(file, "file format not recognized", 0);

	if (shdr[ehdr->e_shstrndx].sh_offset > file_size)
	    return error("DEBUG", "Section header string table offset out of bounds", 0);

	const char *shstrtab = (char*)mapped + shdr[ehdr->e_shstrndx].sh_offset;
	Elf64_Shdr *symtab_hdr = NULL;
	Elf64_Shdr *strtab_hdr = NULL;
	
	for (int i = 0; i < ehdr->e_shnum; i++) {
		const char *name = shstrtab + shdr[i].sh_name;
		if (shdr[i].sh_type == SHT_SYMTAB)
		symtab_hdr = &shdr[i];
		else if (shdr[i].sh_type == SHT_STRTAB && !(ft_strcmp(name, ".strtab")))
		strtab_hdr = &shdr[i];
	}
	
	Elf64_Sym *symbols = (Elf64_Sym *)((char *)mapped + symtab_hdr->sh_offset);
	int num_symbols = symtab_hdr->sh_size / sizeof(Elf64_Sym);
	char *strtab = (char *)mapped + strtab_hdr->sh_offset;
	
	for (int i = 0; i < num_symbols; i++) {
		Elf64_Sym sym = symbols[i];
		unsigned char st_type = ELF64_ST_TYPE(sym.st_info);
	
		const char *name;
		if (st_type == STT_SECTION) {
			name = shstrtab + shdr[sym.st_shndx].sh_name;
		} else {
			if (sym.st_name == 0)
				continue;
			name = strtab + sym.st_name;
		}
	
		char type_char = get_symbol_type_x64(&sym, shdr, ehdr);
		t_header *header = malloc(sizeof(*header));
		if (!header)
			return error("fatal", "malloc allocation failed", 0);
		ft_bzero(header, sizeof(*header));
		header->addr      = sym.st_value;
		header->type_char = type_char;
		header->name      = ft_strdup(name);
		list_push_b(output, node(header, free_header));
	}
	return 0;
}