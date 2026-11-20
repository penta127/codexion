/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	request_before(t_request a, t_request b)
{
	if (a.priority != b.priority)
		return (a.priority < b.priority);
	if (a.tie_breaker != b.tie_breaker)
		return (a.tie_breaker < b.tie_breaker);
	if (a.order != b.order)
		return (a.order < b.order);
	return (a.id < b.id);
}

t_heap	*init_heap(t_config *config)
{
	t_heap	*heap;

	heap = malloc(sizeof(t_heap));
	if (!heap)
		return (NULL);
	heap->data = malloc(sizeof(t_request) * config->coders_number);
	if (!heap->data)
	{
		free(heap);
		return (NULL);
	}
	heap->size = 0;
	heap->capacity = config->coders_number;
	return (heap);
}

void	free_heap(t_heap *heap)
{
	if (!heap)
		return ;
	free(heap->data);
	free(heap);
}

bool	heap_push(t_heap *heap, t_request request)
{
	int			i;
	int			parent;
	t_request	tmp;

	if (!heap || !heap->data || heap->size >= heap->capacity)
		return (false);
	i = heap->size++;
	heap->data[i] = request;
	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (!request_before(heap->data[i], heap->data[parent]))
			break ;
		tmp = heap->data[i];
		heap->data[i] = heap->data[parent];
		heap->data[parent] = tmp;
		i = parent;
	}
	return (true);
}
