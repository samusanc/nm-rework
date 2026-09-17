/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_nm.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/28 08:54:32 by samusanc          #+#    #+#             */
/*   Updated: 2025/05/07 19:27:40 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_NM_H
# define FT_NM_H
# include <stdio.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
# include <sys/mman.h>
# include <stdlib.h>
# include <string.h>
# include <elf.h>
# include "libft.h"

typedef	struct s_header{
	size_t	addr;
	char	type_char;
	char	*name;
}	t_header;

typedef struct s_flags{
	int	a;
	int g;
	int u;
	int	r;
	int p;
	int	total;
	int	error;
} t_flags;

void	print_help();
int		ft_strcmp(const char *str1, const char *str2);
int		error(char *title, char *message, int i);
char	get_symbol_type_x64(Elf64_Sym *sym, Elf64_Shdr *shdrs, Elf64_Ehdr *ehdr);
char	get_symbol_type_x86(Elf32_Sym *sym, Elf32_Shdr *shdrs, Elf32_Ehdr *ehdr);
void	parse_flag(t_flags *result, char *str);
t_flags	parse_flags(char **argv);
void	*free_header(void *ptr);
char	*convert_addr(int addr, int is_64);
void	print_content(t_header *content, int is_64, char flag);
void	print_list(t_list *list, int order, int is_64, char flag);
int		ft_strcmpl(const char *s1, const char *s2);
void	sort_list(t_list *list);
int		process_elf64(void *mapped, t_list *output, size_t file_size, char *file);
int		process_elf32(void *mapped, t_list *output, size_t file_size, char *file);

#endif
