/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/14 11:29:52 by samusanc          #+#    #+#             */
/*   Updated: 2024/08/14 11:30:50 by samusanc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	*free_str(void *str)
{
	free(str);
	return (NULL);
}

int	main()
{
	t_list	hola;


	hola = list(NULL);
	ft_printf("%p, %p, %d\n", hola.head, hola.tail, hola.size);
	list_push_b(&hola, node((void *)ft_strdup("arriba espana"), &free_str));
	ft_printf("%s, %s, %d\n", hola.head->content, hola.tail->content, hola.size);
	list_push_b(&hola, node((void *)ft_strdup("arriba andorra"), &free_str));
	ft_printf("%s, %s, %d\n", hola.head->content, hola.tail->content, hola.size);

}


