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

t_node	*node(void *content, void *(*del)(void *))
{
	t_node	*result;

	result = malloc(sizeof(t_node));
	if (!result)
		return (NULL);
	ft_bzero(result, sizeof(t_node));
	result->content = content;
	result->del = del;
	return (result);
}

void	node_clear(t_node *node)
{
	if (!node)
		return ;
	if (node->del)
		node->del(node->content);
	ft_bzero(node, sizeof(t_node));
	free(node);
}
