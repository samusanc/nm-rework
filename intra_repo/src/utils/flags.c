/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   flags.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/02 02:00:49 by samusanc          #+#    #+#             */
/*   Updated: 2025/05/06 15:52:27 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_nm.h"

void	parse_flag(t_flags *result, char *str)
{
	char	error_str[3];
	char	*error_ptr = NULL;

	if (str[0] != '-')
		return ;
	str++;
	result->total += 1;
	for (int i = 0; str[i]; i++)
	{
		switch (str[i])
		{
			case 'a':
				result->a += 1;
				break;
			case 'g':
				result->g += 1;
				break;
			case 'u':
				result->u += 1;
				break;
			case 'r':
				result->r += 1;
				break;
			case 'p':
				result->p += 1;
				break;
			case 'h':
				print_help();
				exit(0);
				break;
			default:
				error_str[0] = str[i];
				error_str[1] = '\'';
				error_str[2] = '\0';
				error_ptr = ft_strjoin(ft_strdup("invalid option -- '"), ft_strdup(error_str));
				error(NULL, error_ptr, 0);
				free(error_ptr);
				result->error = 1;
				return;
				break;
		}
	}
}

t_flags	parse_flags(char **argv)
{
	t_flags	result;

	ft_bzero(&result, sizeof(t_flags));
	for (int i = 0; argv[i]; i++)
	{
		parse_flag(&result, argv[i]);
		if (result.error)
			return (result);
	}
	return (result);
}

void	*free_header(void *ptr)
{
	t_header	*cast;

	cast = (t_header *)ptr;
	free(cast->name);
	free(ptr);
	return (NULL);
}
