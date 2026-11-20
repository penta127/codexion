/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pop.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	sift_down(t_heap *heap, int i)
{
	int			child;
	t_request	tmp;

	while (2 * i + 1 < heap->size)
	{
		child = 2 * i + 1;
		if (child + 1 < heap->size
			&& request_before(heap->data[child + 1], heap->data[child]))
			child++;
		if (!request_before(heap->data[child], heap->data[i]))
			break ;
		tmp = heap->data[i];
		heap->data[i] = heap->data[child];
		heap->data[child] = tmp;
		i = child;
	}
}

bool	heap_pop(t_heap *heap, t_request *request)
{
	if (!heap || !heap->data || !request || heap->size <= 0)
		return (false);
	*request = heap->data[0];
	heap->size--;
	if (heap->size > 0)
	{
		heap->data[0] = heap->data[heap->size];
		sift_down(heap, 0);
	}
	return (true);
}
