/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libft.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/15 12:31:33 by samusanc          #+#    #+#             */
/*   Updated: 2023/10/19 10:27:02 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_LIST_H
# define FT_LIST_H
# include <unistd.h>
# include <stdlib.h>

typedef struct s_node
{
	void			*content;
	struct s_node	*next;
	struct s_node	*back;
	void			*(*del)(void *);
}				t_node;

typedef struct s_list
{
	unsigned int			size;
	t_node					*head;
	t_node					*tail;
}				t_list;

// node funtions
t_node			*node(void *content, void *(*del)(void *));
void			node_clear(t_node *node);

// list funtions

t_node			*search_head(t_node *node);
t_node			*search_tail(t_node *node);
unsigned int	lst_size(t_node *node);

t_list			list(t_node *node);
void			list_push_f(t_list *list, t_node *node);
void			list_push_b(t_list *list, t_node *node);
void			list_clear(t_list *list);
void			list_del(t_list *list, t_node *node);
void			list_swap(t_node *node_a, t_node *node_b);
t_node			*list_pop(t_list *list, t_node *node);
t_node			*list_find(t_list *list, int *(*f)(void *));

#endif
