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

/*
	WHY THIS CHANGED
	-----------------
	Only one line changed in this function: `if (sym->st_shndx >=
	ehdr->e_shnum) return '?';` was added right before
	`Elf64_Shdr *sec = &shdrs[sym->st_shndx];`. Before this, `st_shndx`
	(a field read straight out of the file, fully attacker/fuzzer
	controlled) was used to index the section header array with zero
	bounds checking. A corrupted or out-of-range st_shndx read an
	Elf64_Shdr-sized chunk of memory from wherever `shdrs + st_shndx`
	happened to land -- past the real section table, inside the mapped
	file if it was big enough, or past the end of the mapping entirely.
	Either way the "section" data read back (sh_type, sh_flags, ...) is
	garbage, and this function's whole job is to turn that into a type
	letter, so garbage in -> garbage (or a crash) out.

	Note this is also why `ehdr` is now genuinely used: the old code
	took `Elf64_Ehdr *ehdr` as a parameter but only ever did
	`(void)ehdr;` on a line of dead code after the function's last
	`return` (i.e. that line could never execute, and every actual
	control path ignored the parameter entirely). Using `ehdr->e_shnum`
	here is what the parameter was clearly meant for from the start.

	SUBJECT COMPLIANCE
	-------------------
	"Be cautious. There are many ways to lead your program out of the
	mapped content, may it be with non-null terminated string, incorrect
	offsets... Check everything." -- st_shndx is exactly this kind of
	"incorrect offset" (an index, not a byte offset, but the same
	principle: never trust a file-supplied position without checking it
	against the actual bound of the array it indexes).

	OLD vs NEW
	----------
	Old: `Elf64_Shdr *sec = &shdrs[sym->st_shndx];` unconditionally.
	New: bounds-checked first, returns '?' (matches the existing "unknown
	     type" fallback already used elsewhere in this same function) for
	     an out-of-range index instead of reading past the array.
*/
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
    if (sym->st_shndx >= ehdr->e_shnum)
        return '?';

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
}

/*
	NEW FUNCTION -- did not exist in the intra submission.

	WHY IT'S NEEDED
	----------------
	The original bounds check in process_elf64() (further down) was:

	    if (ehdr->e_shoff + (ehdr->e_shnum * sizeof(Elf64_Shdr)) > file_size)
	        return error(...);

	`e_shoff` is a 64-bit, file-supplied, fully untrusted offset. If it's
	close to UINT64_MAX, the ADDITION wraps around modulo 2^64 to a small
	number -- small enough to pass the `> file_size` check -- while
	`e_shoff` itself is still an astronomically large value. The very
	next line, `shdr = mapped + ehdr->e_shoff`, then computes a pointer
	that (via the same wraparound, this time in the pointer domain)
	lands BEFORE the start of the mapped buffer. Reproduced with a
	hand-crafted header (`e_shoff = UINT64_MAX - 5`): UBSan caught the
	program reading an Elf64_Shdr from 5 bytes before the mmap'd region
	("misaligned address ... requires 8 byte alignment"). It didn't
	immediately SIGSEGV only because that address happened to still be
	mapped in that particular run; a different allocator/ASLR layout
	turns the same input into an unconditional crash.

	range_in_bounds() replaces every `offset + size > file_size` style
	check in this file with a subtraction-based equivalent that cannot
	wrap: `off > file_size` is checked FIRST (so if it passes, `off` is
	provably <= file_size and `file_size - off` cannot underflow), and
	only then is `size > file_size - off` checked. There is no addition
	of two untrusted values anywhere in this function.

	SUBJECT COMPLIANCE
	-------------------
	Same clause as above: "There are many ways to lead your program out
	of the mapped content ... incorrect offsets ... Check everything."
	This is the textbook case the subject is warning about -- a bounds
	check that LOOKS correct but is defeated by integer overflow.

	OLD vs NEW
	----------
	Old: no such helper; every offset/size pair was checked (or, in the
	     case of .symtab/.strtab, NOT checked at all -- see below) with
	     ad-hoc, overflow-prone addition.
	New: one small, reusable, overflow-safe helper used for the section
	     table itself, .shstrtab, .symtab and .strtab alike.
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
	WHY THIS CHANGED (three separate bugs fixed together, since they're
	all inside the same function and touch overlapping lines)
	--------------------------------------------------------------------

	1) NULL symtab_hdr / strtab_hdr dereference (the single most
	   impactful bug found in the whole project):

	       Elf64_Shdr *symtab_hdr = NULL;
	       Elf64_Shdr *strtab_hdr = NULL;
	       for (...) { ... }              // may leave both still NULL
	       Elf64_Sym *symbols = mapped + symtab_hdr->sh_offset;   // CRASH

	   `symtab_hdr` stays NULL for any file that has no .symtab section
	   -- i.e. any STRIPPED binary. Reproduced directly on real system
	   binaries: `./ft_nm /bin/ls` and
	   `./ft_nm /lib/x86_64-linux-gnu/libc.so.6` both SIGSEGV'd here
	   before this fix, both fixed after it. The subject calls this
	   scenario out by name: "Be aware that on recent Ubuntu versions
	   (20.04+), most system binaries are stripped of symbols." This bug
	   meant the program couldn't even run against the majority of real
	   binaries on the very OS version the subject tells you to test on.
	   Fix: if either header is still NULL after the scan, return 0
	   (success) with an empty `output` list -- that's not a malformed
	   file, it's the normal "this binary has no symbol table" case, and
	   main.c now turns an empty list into the same
	   `nm: file: no symbols` message real nm prints.

	2) The overflow-prone bounds check described above range_in_bounds()
	   -- every place that used to do `offset + size > file_size` now
	   goes through range_in_bounds() instead: the section table itself,
	   .shstrtab, and (new) .symtab and .strtab, which previously had NO
	   bounds check on their sh_offset/sh_size at all before being used.

	3) Unbounded string lookups: `shstrtab + shdr[i].sh_name` (section
	   names) and `strtab + sym.st_name` (symbol names) used to add a
	   file-supplied offset to a base pointer and hand the result
	   straight to ft_strcmp()/ft_strdup(), both of which scan for a
	   NUL byte with no idea where the mapping actually ends. An offset
	   past the table's real size, or a string with no NUL before the
	   mapping's edge, walks straight off into unmapped memory. Fixed by
	   routing every such lookup through safe_str() (src/utils/utils.c),
	   which checks the offset against the table's real size AND
	   confirms a NUL terminator exists before returning a pointer.

	SUBJECT COMPLIANCE
	-------------------
	(1) directly answers "test your program on non-stripped binaries
	(the recommended VM environment)" combined with "In no way can your
	program quit in an unexpected manner" -- the two sentences together
	mean: you WILL be tested against stripped binaries at some point
	(your own machine, a peer's machine, the defense VM), and it MUST
	NOT crash when that happens. (2) and (3) are both the "Check
	everything" / "non-null terminated string / incorrect offsets"
	clause quoted above.

	OLD vs NEW
	----------
	Old: `shdr[ehdr->e_shstrndx].sh_offset > file_size` was the ONLY
	     bounds check for a table offset in this entire function; symtab
	     and strtab offsets, sizes, and every name lookup were
	     completely unchecked, and symtab_hdr/strtab_hdr being NULL was
	     never considered.
	New: every offset+size pair is bounds-checked before use, every
	     string lookup goes through safe_str(), and a missing symtab or
	     strtab is treated as "no symbols" rather than a crash.
*/
int	process_elf64(void *mapped, t_list *output, size_t file_size, char *file)
{
	if (!mapped || !output)
		return 0;
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)mapped;

	if (ehdr->e_shnum == 0)
		return 0;
	if (!range_in_bounds(ehdr->e_shoff, (size_t)ehdr->e_shnum * sizeof(Elf64_Shdr), file_size))
		return error(file, "file format not recognized", 0);

	Elf64_Shdr *shdr = (Elf64_Shdr *)((char *)mapped + ehdr->e_shoff);

	if (ehdr->e_shstrndx >= ehdr->e_shnum)
		return error(file, "file format not recognized", 0);
	if (!range_in_bounds(shdr[ehdr->e_shstrndx].sh_offset, shdr[ehdr->e_shstrndx].sh_size, file_size))
		return error(file, "file format not recognized", 0);

	size_t shstrtab_size = shdr[ehdr->e_shstrndx].sh_size;
	const char *shstrtab = (char*)mapped + shdr[ehdr->e_shstrndx].sh_offset;
	Elf64_Shdr *symtab_hdr = NULL;
	Elf64_Shdr *strtab_hdr = NULL;

	for (int i = 0; i < ehdr->e_shnum; i++) {
		const char *name = safe_str(shstrtab, shdr[i].sh_name, shstrtab_size);
		if (shdr[i].sh_type == SHT_SYMTAB)
			symtab_hdr = &shdr[i];
		else if (shdr[i].sh_type == SHT_STRTAB && name && !(ft_strcmp(name, ".strtab")))
			strtab_hdr = &shdr[i];
	}

	/* A missing .symtab/.strtab is NOT a malformed file -- it's a
	   normal stripped binary. See the big comment above this function
	   for why this branch is the fix for the most severe bug found. */
	if (!symtab_hdr || !strtab_hdr)
		return 0;
	if (!range_in_bounds(symtab_hdr->sh_offset, symtab_hdr->sh_size, file_size))
		return error(file, "file format not recognized", 0);
	if (!range_in_bounds(strtab_hdr->sh_offset, strtab_hdr->sh_size, file_size))
		return error(file, "file format not recognized", 0);

	Elf64_Sym *symbols = (Elf64_Sym *)((char *)mapped + symtab_hdr->sh_offset);
	size_t num_symbols = symtab_hdr->sh_size / sizeof(Elf64_Sym);
	char *strtab = (char *)mapped + strtab_hdr->sh_offset;
	size_t strtab_size = strtab_hdr->sh_size;

	for (size_t i = 0; i < num_symbols; i++) {
		Elf64_Sym sym = symbols[i];
		unsigned char st_type = ELF64_ST_TYPE(sym.st_info);
		/*
			WHY THIS CHECK EXISTS
			-----------------------
			Index 0 of every ELF symtab is a reserved, all-zero "null"
			entry (name/value/size/info all 0, st_shndx == SHN_UNDEF).
			Under normal (non -a) output it's correctly invisible --
			it falls into the `sym.st_name == 0 -> continue` branch
			below like any other nameless symbol. But real nm's -a
			flag ("Display debugger-only symbols") surfaces this exact
			entry as an empty-named ABSOLUTE symbol ('a'), not as 'U'
			(which is what get_symbol_type_x64() would normally return
			for SHN_UNDEF + STB_LOCAL). This is a libbfd implementation
			quirk, not something derivable from the ELF spec, and it
			ONLY happens for linked files (exe/.so) -- verified against
			real nm that a plain relocatable .o does NOT get this
			extra line under -a. Reproduced/matched by diffing
			`nm -a` against `ft_nm -a` on real 32-bit and 64-bit
			exe/.o/.so files until every one matched byte-for-byte.
		*/
		int is_reserved_null = (i == 0 && sym.st_name == 0 && sym.st_value == 0
			&& sym.st_shndx == SHN_UNDEF && st_type == STT_NOTYPE
			&& ehdr->e_type != ET_REL);

		const char *name;
		if (st_type == STT_SECTION) {
			/* st_shndx bounds-checked before indexing shdr[]; see the
			   matching fix in get_symbol_type_x64() above for why. */
			if (sym.st_shndx >= ehdr->e_shnum)
				continue;
			name = safe_str(shstrtab, shdr[sym.st_shndx].sh_name, shstrtab_size);
		} else if (is_reserved_null) {
			name = "";
		} else {
			if (sym.st_name == 0)
				continue;
			/* safe_str() instead of raw `strtab + sym.st_name`: see
			   point (3) in the comment above this function. */
			name = safe_str(strtab, sym.st_name, strtab_size);
		}
		if (!name)
			continue;

		char type_char = is_reserved_null ? 'a' : get_symbol_type_x64(&sym, shdr, ehdr);
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
