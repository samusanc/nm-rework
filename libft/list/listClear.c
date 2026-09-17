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

/*
	WHY THIS CHANGED
	-----------------
	list_del() (libft/list/listRmv.c) removes a single node by walking
	the list FROM THE HEAD to find it: `tmp = list->head; while (tmp !=
	node && tmp) tmp = tmp->next;`. The old list_clear() called
	list_del() once per node to tear the whole list down -- so freeing
	N nodes did an O(N) head-to-node search N times, i.e. O(N^2) total,
	just to release memory that was about to be thrown away anyway.
	Paired with the O(n^3) sort_list() bug (src/utils/utils.c, fixed in
	the same commit as this file), this made a large symbol table's
	full processing time blow up badly: the sort was the dominant cost,
	but this made teardown of the same list nontrivially slow too, and
	both had to be fixed together to actually solve the hang.

	Fixed by walking the list once, front to back, freeing each node
	directly with node_clear() (which is what list_del() would have
	called anyway, after finding the node) -- no search needed, because
	when the WHOLE list is being discarded there's nothing to search
	for; every node gets freed regardless of position.

	SUBJECT COMPLIANCE
	-------------------
	Same clause as sort_list()'s fix: "never crash" has to include
	"never grind to a halt on realistic input" (a symbol table with a
	few thousand entries is not an edge case for a real binary).

	OLD vs NEW
	----------
	Old: `list_del(list, tmp)` per node -- O(n) search, called n times.
	New: direct `node_clear(tmp)` per node in a single O(n) pass, no
	     search.
*/
void	list_clear(t_list *list)
{
	t_node			*tmp;
	t_node			*next;

	tmp = list->head;
	while (tmp)
	{
		next = tmp->next;
		node_clear(tmp);
		tmp = next;
	}
	list->size = 0;
	list->head = 0;
	list->tail = 0;
}
