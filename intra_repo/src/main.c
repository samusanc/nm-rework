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

/**
 * Verifies if the file is a valid ELF file
 * Checks the magic number at the beginning of the file
 */
int	is_valid_elf(unsigned char *e_ident, char *file)
{
	if (e_ident[0] != 0x7f || e_ident[1] != 'E' || e_ident[2] != 'L' || e_ident[3] != 'F')
		return error(file, "file format not recognized", 0);
	return 0;
}

int	ft_nm(char *file, t_flags flags, int multiple)
{
	if (file[0] == '-')
		return 0;
	
	int fd = open(file, O_RDONLY);
	if (fd < 0)
		return error(file, "No such file", 1);
	
	struct stat st;
	fstat(fd, &st);
	void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	
	unsigned char *e_ident = (unsigned char *)mapped;
	if (is_valid_elf(e_ident, file))
		return 0;

	int is_64 = (e_ident[4] == ELFCLASS64);
	t_list output = list(NULL);
	
	if (multiple)
		ft_printf("\n%s:\n", file);

	int result = 0;
	if (is_64)
		result = process_elf64(mapped, &output, st.st_size, file);
	else
		result = process_elf32(mapped, &output, st.st_size, file);
	
	if (result == 0)
		display_symbols(&output, flags, is_64);
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
