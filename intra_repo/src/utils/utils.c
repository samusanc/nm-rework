/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/02 02:01:20 by samusanc          #+#    #+#             */
/*   Updated: 2025/05/14 18:48:10 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_nm.h"

void	print_help()
{
	ft_printf("%s", "Usage: nm [option(s)] [file(s)]\n");
	ft_printf("%s", " List symbols in [file(s)] (a.out by default).\n");
	ft_printf("%s", " The options are:\n");
	ft_printf("%s", "  -a, --debug-syms       Display debugger-only symbols\n");
	ft_printf("%s", "  -A, --print-file-name  Print name of the input file before every symbol\n");
	ft_printf("%s", "  -B                     Same as --format=bsd\n");
	ft_printf("%s", "  -C, --demangle[=STYLE] Decode mangled/processed symbol names\n");
	ft_printf("%s", "                           STYLE can be \"none\", \"auto\", \"gnu-v3\", \"java\",\n");
	ft_printf("%s", "                           \"gnat\", \"dlang\", \"rust\"\n");
	ft_printf("%s", "      --no-demangle      Do not demangle low-level symbol names\n");
	ft_printf("%s", "      --recurse-limit    Enable a demangling recursion limit.  (default)\n");
	ft_printf("%s", "      --no-recurse-limit Disable a demangling recursion limit.\n");
	ft_printf("%s", "  -D, --dynamic          Display dynamic symbols instead of normal symbols\n");
	ft_printf("%s", "  -e                     (ignored)\n");
	ft_printf("%s", "  -f, --format=FORMAT    Use the output format FORMAT.  FORMAT can be `bsd',\n");
	ft_printf("%s", "                           `sysv', `posix' or 'just-symbols'.\n");
	ft_printf("%s", "                           The default is `bsd'\n");
	ft_printf("%s", "  -g, --extern-only      Display only external symbols\n");
	ft_printf("%s", "    --ifunc-chars=CHARS  Characters to use when displaying ifunc symbols\n");
	ft_printf("%s", "  -j, --just-symbols     Same as --format=just-symbols\n");
	ft_printf("%s", "  -l, --line-numbers     Use debugging information to find a filename and\n");
	ft_printf("%s", "                           line number for each symbol\n");
	ft_printf("%s", "  -n, --numeric-sort     Sort symbols numerically by address\n");
	ft_printf("%s", "  -o                     Same as -A\n");
	ft_printf("%s", "  -p, --no-sort          Do not sort the symbols\n");
	ft_printf("%s", "  -P, --portability      Same as --format=posix\n");
	ft_printf("%s", "  -r, --reverse-sort     Reverse the sense of the sort\n");
	ft_printf("%s", "      --plugin NAME      Load the specified plugin\n");
	ft_printf("%s", "  -S, --print-size       Print size of defined symbols\n");
	ft_printf("%s", "  -s, --print-armap      Include index for symbols from archive members\n");
	ft_printf("%s", "      --quiet            Suppress \"no symbols\" diagnostic\n");
	ft_printf("%s", "      --size-sort        Sort symbols by size\n");
	ft_printf("%s", "      --special-syms     Include special symbols in the output\n");
	ft_printf("%s", "      --synthetic        Display synthetic symbols as well\n");
	ft_printf("%s", "  -t, --radix=RADIX      Use RADIX for printing symbol values\n");
	ft_printf("%s", "      --target=BFDNAME   Specify the target object format as BFDNAME\n");
	ft_printf("%s", "  -u, --undefined-only   Display only undefined symbols\n");
	ft_printf("%s", "  -U, --defined-only     Display only defined symbols\n");
	ft_printf("%s", "      --unicode={default|show|invalid|hex|escape|highlight}\n");
	ft_printf("%s", "                         Specify how to treat UTF-8 encoded unicode characters\n");
	ft_printf("%s", "  -W, --no-weak          Ignore weak symbols\n");
	ft_printf("%s", "      --with-symbol-versions  Display version strings after symbol names\n");
	ft_printf("%s", "  -X 32_64               (ignored)\n");
	ft_printf("%s", "  @FILE                  Read options from FILE\n");
	ft_printf("%s", "  -h, --help             Display this information\n");
	ft_printf("%s", "  -V, --version          Display this program's version number\n");
	ft_printf("%s", "nm: supported targets: elf64-x86-64 elf32-i386 elf32-iamcu elf32-x86-64 pei-i386 pe-x86-64 pei-x86-64 elf64-little elf64-big elf32-little elf32-big pe-bigobj-x86-64 pe-i386 pdb srec symbolsrec verilog tekhex binary ihex plugin");
}

int	ft_strcmp(const char *str1, const char *str2)
{
	int	len1 = strlen(str1);
	int	len2 = strlen(str2);

	if (len1 > len2)
		return (ft_strncmp(str1, str2, len1));
	else
		return (ft_strncmp(str1, str2, len2));
}

int	error(char *title, char *message, int i)
{
	write(2, "nm: ", 4);
    if (title)
    {
        if (i)
            write(2, "'", 1);
        write(2, title, ft_strlen(title));
        if (i)
            write(2, "'", 1);
        write(2, ": ", 2);
    }
    write(2, message, ft_strlen(message));
	write(2, "\n", 1);
	return (1);
}

int ft_strcmpl(const char *s1, const char *s2)
{
    unsigned char c1, c2;

    while (*s1 != '\0' || *s2 != '\0')
    {
        while (*s1 == '_') s1++;
        while (*s2 == '_') s2++;

        if (*s1 == '\0' && *s2 == '\0')
            return 0;

        c1 = (unsigned char)ft_tolower(*s1);
        c2 = (unsigned char)ft_tolower(*s2);

        if (c1 != c2)
            return (int)c1 - (int)c2;

        if (*s1) s1++;
        if (*s2) s2++;
    }
    return 0;
}

void	sort_list(t_list *list)
{
	t_node	*tmp1;
	tmp1 = list->head;
	for (int i = 0; (size_t)i < list->size; i++)
	{
		t_node	*tmp2 = tmp1->next;
		if (tmp2)
		{
			t_header	*content1 = tmp1->content;
			t_header	*content2 = tmp2->content;
			if (ft_strcmp(content1->name, content2->name) < 0)
			{
				list_swap(tmp1, tmp2);
				i = -1;
				tmp1 = list->head;
				continue;
			}
		}
		tmp1 = tmp2;
	}
}
