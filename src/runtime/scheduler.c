/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scheduler.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static bool	try_request(t_shared *s, t_request request)
{
	t_coder	*c;
	long	now;

	c = &s->coders[request.id - 1];
	now = now_ms();
	if (now < c->s_compile_time + s->config->burnout_time
		&& dongles_available(s, c, now))
	{
		grant_dongles(s, c);
		return (true);
	}
	s->dongles[c->left_dongle_number].blocked = true;
	s->dongles[c->right_dongle_number].blocked = true;
	return (false);
}

void	dispatch_requests(t_shared *s)
{
	t_request	request;
	int			count;
	int			i;
	bool		granted;

	i = 0;
	while (i < s->config->coders_number)
		s->dongles[i++].blocked = false;
	count = 0;
	granted = false;
	while (heap_pop(s->heap, &request))
	{
		if (!try_request(s, request))
			s->pending[count++] = request;
		else
			granted = true;
	}
	i = 0;
	while (i < count)
		heap_push(s->heap, s->pending[i++]);
	if (granted)
		pthread_cond_broadcast(&s->changed);
}

static t_request	make_request(t_coder *coder)
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
	return (request);
}

bool	acquire_dongles(t_coder *coder)
{
	t_shared	*s;
	t_request	request;
	bool		acquired;

	s = coder->shared;
	pthread_mutex_lock(&s->mutex_stop);
	request = make_request(coder);
	if (!s->stopped && !heap_push(s->heap, request))
		s->stopped = true;
	while (!s->stopped && !coder->granted)
	{
		dispatch_requests(s);
		if (!coder->granted)
			wait_change(s);
	}
	acquired = coder->granted;
	pthread_mutex_unlock(&s->mutex_stop);
	return (acquired);
}
