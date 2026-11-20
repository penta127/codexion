/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	finish_compile(t_coder *coder)
{
	t_shared	*s;
	bool		done;
	bool		expired;

	s = coder->shared;
	pthread_mutex_lock(&s->mutex_stop);
	expired = now_ms() - coder->s_compile_time >= s->config->burnout_time;
	release_dongles(coder);
	if (!s->stopped && !expired)
		coder->count_compile++;
	done = s->stopped || expired
		|| coder->count_compile >= s->config->required_number;
	pthread_cond_broadcast(&s->changed);
	pthread_mutex_unlock(&s->mutex_stop);
	return (done);
}

static bool	work_cycle(t_coder *coder)
{
	t_shared	*s;

	s = coder->shared;
	if (!acquire_dongles(coder))
		return (false);
	wait_until(s, coder->s_compile_time + s->config->compile_time);
	if (finish_compile(coder))
		return (false);
	if (!log_action(coder, "is debugging")
		|| !wait_until(s, now_ms() + s->config->debug_time))
		return (false);
	if (!log_action(coder, "is refactoring")
		|| !wait_until(s, now_ms() + s->config->refactor_time))
		return (false);
	return (true);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = arg;
	if (!prepare_coder_start(coder))
		return (NULL);
	if (coder->shared->config->coders_number == 1)
	{
		log_action(coder, "has taken a dongle");
		wait_until(coder->shared, coder->s_compile_time
			+ coder->shared->config->burnout_time + 1L);
		return (NULL);
	}
	while (work_cycle(coder))
		;
	return (NULL);
}
