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
#include "libft.h"

static t_node	*one_exist(t_node *a, t_node *b)
{
	if (a)
		return (a);
	return (b);
}

static void	list_disconnect(t_list *list, t_node *node)
{
	t_node	*tmp_b;
	t_node	*tmp_n;

	tmp_b = node->back;
	tmp_n = node->next;
	node->back = NULL;
	node->next = NULL;
	list->size--;
	if (tmp_b)
		tmp_b->next = tmp_n;
	if (tmp_n)
		tmp_n->back = tmp_b;
	if (!tmp_b && !tmp_n)
		ft_bzero(list, sizeof(t_list));
	else
	{
		list->head = search_head(one_exist(tmp_b, tmp_n));
		list->tail = search_tail(one_exist(tmp_b, tmp_n));
	}
}

/*
	delete a node from a list
*/
void	list_del(t_list *list, t_node *node)
{
	t_node	*tmp;

	tmp = list->head;
	while (tmp != node && tmp)
		tmp = tmp->next;
	if (!tmp)
		return ;
	list_disconnect(list, tmp);
	node_clear(tmp);
	tmp = NULL;
}

/*
	extract a node from a list
*/
t_node	*list_pop(t_list *list, t_node *node)
{
	t_node	*tmp;

	tmp = list->head;
	while (tmp != node && tmp)
		tmp = tmp->next;
	if (!tmp)
		return (NULL);
	list_disconnect(list, tmp);
	return (tmp);
}
