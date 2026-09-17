/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   list.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/14 11:24:18 by samusanc          #+#    #+#             */
/*   Updated: 2024/08/14 11:24:27 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_list.h"
#include <stdio.h>

void	list_clear(t_list *list)
{
	t_node			*tmp;

	tmp = list->tail;
	while (tmp)
	{
		list_del(list, tmp);
		tmp = list->tail;
	}
	list->size = 0;
	list->head = 0;
}
