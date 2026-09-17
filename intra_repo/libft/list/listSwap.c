/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   listSwap.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/14 11:24:18 by samusanc          #+#    #+#             */
/*   Updated: 2024/08/14 13:53:07 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_list.h"

/*
	swap the content of nodes
*/
void	list_swap(t_node *node_a, t_node *node_b)
{
	void	*a;
	void	*b;
	void	*(*del_a)(void *);
	void	*(*del_b)(void *);

	a = node_a->content;
	b = node_b->content;
	del_a = node_a->del;
	del_b = node_b->del;
	node_a->content = b;
	node_b->content = a;
	node_a->del = del_b;
	node_b->del = del_a;
}
