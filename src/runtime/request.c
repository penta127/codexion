/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	enqueue_request(t_coder *coder)
{
	t_request	request;
	t_shared	*s;

	s = coder->shared;
	request.id = coder->id;
	request.priority = 0;
	request.tie_breaker = 0;
	request.order = s->next_order++;
	if (strcmp(s->config->scheduler, "edf") == 0)
	{
		request.priority = coder->s_compile_time + s->config->burnout_time;
		request.tie_breaker = -coder->id;
	}
	if (!heap_push(s->heap, request))
		return (false);
	coder->queued = true;
	return (true);
}
