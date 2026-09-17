/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/02 02:01:04 by samusanc          #+#    #+#             */
/*   Updated: 2025/05/07 19:10:54 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_nm.h"

/*
	WHY THIS CHANGED
	-----------------
	Old signature was `convert_addr(int addr, int is_64)` and it called
	`ft_itoa(addr)` -- ft_itoa() is a DECIMAL integer-to-string
	converter. Every nonzero symbol address in the entire program's
	output was printed in base 10 instead of base 16. Proven directly
	against real `nm` on bin/libmy_library.so:

	    real nm:  0000000000003e08 d _DYNAMIC
	    old ft_nm: 0000000000015880 d _DYNAMIC

	  0x3e08 (hex) == 15880 (decimal) -- the exact same number, wrong
	  base. This survived the ENTIRE original test suite because both
	  tester.sh and diff.sh stripped the address column before diffing
	  (`awk '{print $2, $3}'` / `parts[1:3]`), so the tests only ever
	  compared the type letter and the name, never the address. See the
	  matching fix to tester.sh/diff.sh in this same series of commits.

	  Stacked on top: the parameter was `int addr`, but the real value
	  (t_header.addr, see includes/ft_nm.h) is a 64-bit `size_t`. Every
	  call was truncating the top 32 bits of the address before even
	  reaching the (also-wrong) decimal conversion.

	SUBJECT COMPLIANCE
	-------------------
	"Output is to be similar to nm on the symbols list (order, offset,
	padding...)." -- the address IS the offset. Printing it in the wrong
	numeric base isn't a cosmetic difference, it's the output being
	factually wrong for every single symbol that isn't at address 0.

	OLD vs NEW
	----------
	Old: `int addr` parameter (truncates 64-bit addresses) fed through
	     `ft_itoa()` (decimal) and hand-padded into fixed-width buffers.
	New: `size_t addr` parameter (no truncation), converted digit-by-
	     digit into hex nibbles directly into a correctly-sized,
	     zero-padded buffer (16 hex digits for 64-bit, 8 for 32-bit,
	     matching real nm's column width exactly).
*/
char	*convert_addr(size_t addr, int is_64)
{
	static const char	digits[] = "0123456789abcdef";
	int					width;
	char				*buf;
	int					i;

	width = is_64 ? 16 : 8;
	buf = malloc(width + 1);
	if (!buf)
		return (NULL);
	i = width - 1;
	while (i >= 0)
	{
		buf[i] = digits[addr & 0xf];
		addr >>= 4;
		i--;
	}
	buf[width] = '\0';
	return (buf);
}

void	print_content(t_header *content, int is_64, char flag)
{
	if (flag != 'a')
		if (content->type_char == 'a' || content->type_char == 'A' || content->type_char == 'N' || content->name[0] == '.')
			return ;
	if (flag == 'u')
		if (content->type_char != 'U' && content->type_char != 'w')
			return ;
	if (flag == 'g')
		if (content->type_char == 't' || content->type_char == 'd' || content->type_char == 'b' || content->type_char == 'r')
			return ;
	/*
		WHY THIS CHANGED
		-----------------
		Old condition was a POSITIVE WHITELIST of type letters that get
		an address printed even when the value is 0
		(T/t/a/N/b/D/r, plus "any type if addr != 0"). It's incomplete:
		'd' (local data), 'R' (global read-only), 'A' (global absolute),
		'c'/'C' (common) are all missing. A symbol of one of THOSE types
		whose value happens to be exactly 0 fell into the `else` branch
		below and printed blank padding instead of an address.
		Reproduced on a real compiled .o file: a `.data`-section local
		symbol at address 0 -- real nm printed
		`00000000 d .data`, the old code printed `d .data` (blank).

		Fixed by inverting the logic to a BLACKLIST of the only two
		types that genuinely have no address to show: 'U' (undefined)
		and 'w' (weak-undefined) -- an undefined symbol has no value by
		definition, which is the one case real nm actually leaves blank.
		Every other type, including a legitimately zero-valued one, now
		gets its address printed.

		SUBJECT COMPLIANCE
		-------------------
		"Output is to be similar to nm on the symbols list (order,
		offset, padding...)." -- silently blanking out a real (if zero)
		address for certain symbol types is exactly the kind of output
		mismatch this line is about.

		OLD vs NEW
		----------
		Old: enumerate every type that SHOULD get an address (easy to
		     leave one out, and it fails silently -- no warning, no
		     crash, just a wrong-looking but plausible line of output).
		New: enumerate the two types that should NOT (a much smaller,
		     closed set, per the ELF spec: only undefined symbols lack
		     a meaningful value).
	*/
	if (content->type_char != 'U' && content->type_char != 'w')
	{
		if (ft_strcmp(content->name, ".comment") == 0)
			content->type_char = 'n';
		char	*str = convert_addr(content->addr, is_64);
		ft_printf("%s %c %s\n", str, content->type_char, content->name);
		free(str);
	}
	else
	{
		if (is_64)
			ft_printf("                 %c %s\n", content->type_char, content->name);
		else
			ft_printf("         %c %s\n", content->type_char, content->name);
	}
}

void	print_list(t_list *list, int order, int is_64, char flag)
{
		t_node	*tmp;

		if (order)
			tmp = list->head;
		else
			tmp = list->tail;
		for (int i = 0; (size_t)i < list->size; i++)
		{
			t_header	*tmp2;

			tmp2 = (t_header *)tmp->content;
			print_content(tmp2, is_64, flag);
			if (order)
				tmp = tmp->next;
			else
				tmp = tmp->back;
		}

}
