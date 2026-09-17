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

void	list_push_f(t_list *list, t_node *node)
{
	t_node	*tmp;

	if (!node || !list)
		return ;
	tmp = list->head;
	if (tmp)
	{
		tmp->back = node;
		node->next = tmp;
		node->back = NULL;
		list->head = node;
	}
	else
	{
		list->head = node;
		list->tail = node;
		list->size = 0;
	}
	list->size++;
}

void	list_push_b(t_list *list, t_node *node)
{
	t_node	*tmp;

	if (!node || !list)
		return ;
	tmp = list->tail;
	if (tmp)
	{
		tmp->next = node;
		node->back = tmp;
		node->next = NULL;
		list->tail = node;
	}
	else
	{
		list->head = node;
		list->tail = node;
		list->size = 0;
	}
	list->size++;
}
