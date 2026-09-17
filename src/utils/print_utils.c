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

char	*convert_addr(int addr, int is_64)
{
	char	str_64[17];
	char	str_86[9];

	ft_memset(str_64, '0', sizeof(str_64));
	ft_memset(str_86, '0', sizeof(str_86));

	char	*nbr = ft_itoa(addr);
	if (!nbr)
		return (NULL);
	str_64[16 - ft_strlen(nbr)] = '\0';
	str_86[8 - ft_strlen(nbr)] = '\0';
	if (is_64)
		return (ft_strjoin(ft_strdup(str_64), nbr));
	else
		return (ft_strjoin(ft_strdup(str_86), nbr));
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
	if (content->addr || 
		content->type_char == 'T' || 
		content->type_char == 't' || 
		content->type_char == 'a' || 
		content->type_char == 'N' || 
		content->type_char == 'b' || 
		content->type_char == 'D' || 
		content->type_char == 'r')
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
