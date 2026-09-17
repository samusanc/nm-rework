/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/28 08:18:02 by samusanc          #+#    #+#             */
/*   Updated: 2025/05/07 19:28:02 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_nm.h"
#include "libft.h"

void	display_symbols(t_list *output, t_flags flags, int is_64)
{
	int reverse = 0;

	if (!flags.p) {
		sort_list(output);
		if (flags.r)
			reverse = 1;
	}
	else
		reverse = 1;

	char format = 'x';
	if (flags.u)
		format = 'u';
	else if (flags.g)
		format = 'g';
	else if (flags.a)
		format = 'a';
	print_list(output, reverse, is_64, format);
}

/*
	WHY THIS CHANGED
	-----------------
	Original version only checked the 4 magic bytes (0x7f 'E' 'L' 'F') and,
	on failure, returned the SAME value (1, via error()) that a caller would
	read as "print error, count it as a failure" -- but the caller (ft_nm())
	used to do `if (is_valid_elf(...)) return 0;`, i.e. it treated "this is
	NOT a valid ELF" as ft_nm() SUCCEEDING (return 0 -> not added to
	error_counter). So a text file or a garbage file would print
	"file format not recognized" to stderr but the process would still exit
	0, as if nothing went wrong. Real `nm` exits 1 for that (verified:
	`nm garbage.txt` -> exit 1). ft_nm()'s caller was fixed to propagate 1
	instead (see the `int ft_nm(...)` block below), and this function was
	extended at the same time to also validate e_ident[4] (the ELF class
	byte, EI_CLASS): the old code just did
	`is_64 = (e_ident[4] == ELFCLASS64)` further down with no validation,
	meaning ANY byte that wasn't exactly 2 silently fell through the
	32-bit path -- including a corrupted/garbage class byte.

	SUBJECT COMPLIANCE
	-------------------
	"Make sure that every value that you parse is correct: for instance,
	when parsing flags, architecture, match your result with a reference
	to ensure its value is correct." -- e_ident[4] is exactly this kind of
	value: it has exactly two legal values (ELFCLASS32 / ELFCLASS64) and
	everything else must be rejected up front, not silently coerced into
	one of the two branches.

	OLD vs NEW
	----------
	Old: only checks magic, returns 0 (bool-ish) and lets the wrong bit
	     survive as "success" at the call site.
	New: checks magic AND class, calls error() itself, and returns a
	     value the caller now actually trusts (1 == reject, 0 == ok).
*/
int	is_valid_elf(unsigned char *e_ident, char *file)
{
	if (e_ident[0] != 0x7f || e_ident[1] != 'E' || e_ident[2] != 'L' || e_ident[3] != 'F')
	{
		error(file, "file format not recognized", 0);
		return 1;
	}
	if (e_ident[4] != ELFCLASS32 && e_ident[4] != ELFCLASS64)
	{
		error(file, "file format not recognized", 0);
		return 1;
	}
	return 0;
}

/*
	NEW FUNCTION -- did not exist in the intra submission.
	Needed once ft_nm() below started classifying the input file with
	stat()/fstat() instead of blindly mmap()'ing it: a directory is a
	perfectly openable, fstat()-able file, so without an explicit check
	it would sail past every earlier guard and get handed to mmap(),
	which fails for a directory and (before this whole pass) had its
	return value ignored -- see the SEGFAULT note on ft_nm() below.
	Real `nm` prints `nm: Warning: 'x' is a directory` and exits 1 for
	this case; this reproduces that message.
*/
static int	warn_directory(char *file)
{
	write(2, "nm: Warning: '", 14);
	write(2, file, ft_strlen(file));
	write(2, "' is a directory\n", 17);
	return 1;
}

/*
	WHY THIS CHANGED (the big one)
	--------------------------------
	The original body was:

	    int fd = open(file, O_RDONLY);
	    if (fd < 0)
	        return error(file, "No such file", 1);
	    struct stat st;
	    fstat(fd, &st);                          // return value ignored
	    void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	    close(fd);
	    unsigned char *e_ident = (unsigned char *)mapped;   // used unchecked
	    if (is_valid_elf(e_ident, file))
	        return 0;

	Two syscalls here can fail and neither failure was ever checked:
	  - fstat() can fail (rare, but possible) leaving `st` uninitialised,
	    so st.st_size would be garbage.
	  - mmap() returns MAP_FAILED == (void *)-1 on failure, and it WILL
	    fail for: an empty file (mmap length 0 is invalid), and a
	    directory's fd (mmap on a directory is rejected by the kernel).
	  In both cases `mapped` becomes (void *)-1, and the very next line
	  dereferences it as `e_ident[0]` inside is_valid_elf() -> guaranteed
	  SIGSEGV. Reproduced live with ASan on both `./ft_nm empty_file` and
	  `./ft_nm some_directory` -- both crashed at is_valid_elf, main.c:44
	  (the line number in the *old* file), confirming this exact path.

	SUBJECT COMPLIANCE
	-------------------
	"Your program must behave like the system nm on every other aspect.
	You have to handle errors carefully. In no way can your program quit
	in an unexpected manner (Segmentation fault, bus error, double free,
	etc.)." -- this was a direct, unconditional violation: any of "empty
	file" / "directory as argument" / (as root) "0-byte permission-000
	file" crashed the program outright, no flags or special input needed.

	OLD vs NEW
	----------
	Old: open() -> fstat() [unchecked] -> mmap() [unchecked] -> dereference.
	New: open() -> fstat() [checked] -> reject directories -> reject files
	     smaller than the smallest possible ELF header -> mmap() [checked
	     against MAP_FAILED] -> only THEN dereference. Also added the
	     e_machine (architecture) check and the "no symbols" branch (see
	     inline comments below) in this same pass, since they touch the
	     same function and the same subject requirement about validating
	     every parsed value against a reference.
*/
int	ft_nm(char *file, t_flags flags, int multiple)
{
	if (file[0] == '-')
		return 0;

	int fd = open(file, O_RDONLY);
	if (fd < 0)
		return error(file, "No such file", 1);

	struct stat st;
	if (fstat(fd, &st) < 0)
	{
		close(fd);
		return error(file, "No such file", 1);
	}
	if (S_ISDIR(st.st_mode))
	{
		close(fd);
		return warn_directory(file);
	}
	/* Reject anything smaller than the smallest possible ELF header
	   (32-bit) up front. This also covers the empty-file case (size 0)
	   without needing a separate check, and it means process_elf32()
	   never has to worry about ehdr->e_ident being partially out of the
	   mapped file. */
	if (st.st_size <= 0 || (size_t)st.st_size < sizeof(Elf32_Ehdr))
	{
		close(fd);
		return error(file, "file format not recognized", 0);
	}

	void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (mapped == MAP_FAILED)
		return error(file, "file format not recognized", 0);

	unsigned char *e_ident = (unsigned char *)mapped;
	if (is_valid_elf(e_ident, file))
	{
		munmap(mapped, st.st_size);
		return 1;
	}

	int is_64 = (e_ident[4] == ELFCLASS64);
	/* is_valid_elf() already proved e_ident[4] is 32 or 64 bit class; if
	   it's 64-bit, also prove the file is at least large enough to hold
	   a full 64-bit header (it was already checked against the smaller
	   32-bit header above, which isn't enough for a 64-bit file). */
	if (is_64 && (size_t)st.st_size < sizeof(Elf64_Ehdr))
	{
		munmap(mapped, st.st_size);
		return error(file, "file format not recognized", 0);
	}

	/*
		NEW CHECK -- did not exist in the intra submission at all.
		The subject: "Make sure that every value that you parse is
		correct: for instance, when parsing flags, architecture, match
		your result with a reference to ensure its value is correct."
		The old code only ever looked at e_ident[4] (32 vs 64-bit class)
		and never checked e_machine, so a file claiming to be the right
		class but built for a completely different CPU architecture
		(the `wrong_arch` test fixture in bin/ is exactly this: 32-bit
		class, e_machine says x86-64) would still be parsed as if it
		were valid. e_machine sits at the same struct offset in both
		Elf32_Ehdr and Elf64_Ehdr, so it can be read through either type
		before we've committed to one.
	*/
	Elf32_Half e_machine = ((Elf32_Ehdr *)mapped)->e_machine;
	if ((is_64 && e_machine != EM_X86_64) || (!is_64 && e_machine != EM_386))
	{
		munmap(mapped, st.st_size);
		return error(file, "file format not recognized", 0);
	}

	t_list output = list(NULL);

	if (multiple)
		ft_printf("\n%s:\n", file);

	int result = 0;
	if (is_64)
		result = process_elf64(mapped, &output, st.st_size, file);
	else
		result = process_elf32(mapped, &output, st.st_size, file);

	/*
		NEW: "no symbols" handling. process_elf64()/process_elf32() were
		changed (see src/x64/x64_utils.c) to return success (0) with an
		EMPTY output list instead of crashing when a binary legitimately
		has no .symtab (i.e. it's stripped -- true for the vast majority
		of real system binaries, per the subject's own warning about
		Ubuntu 20.04+). display_symbols() on an empty list would just
		print nothing, which does NOT match real nm: real `nm` prints
		`nm: file: no symbols` and still exits 0. This branch reproduces
		that exact message/exit-code pair (verified against
		`nm /bin/ls` and `nm libc.so.6`, both stripped).
	*/
	if (result == 0)
	{
		if (output.size == 0)
			error(file, "no symbols", 0);
		else
			display_symbols(&output, flags, is_64);
	}
	list_clear(&output);
	munmap(mapped, st.st_size);
	return result;
}

int	main(int argc, char **argv)
{
	int		error_counter = 0;
	t_flags	flags = parse_flags(argv);

	if (flags.error && flags.error != -1)
	{
		print_help();
		return 1;
	}
	if (argc == (flags.total + 1))
	{
		if (argc > (flags.total + 1))
			error_counter += ft_nm("a.out", flags, 0);
		else
			error_counter += ft_nm("a.out", flags, 1);
	}
	else
	{
		for (int i = 1; i < argc; i++) {
			if (argc > (flags.total + 1))
				error_counter += ft_nm(argv[i], flags, 0);
			else
				error_counter += ft_nm(argv[i], flags, 1);

		}
	}
	return error_counter;
}
