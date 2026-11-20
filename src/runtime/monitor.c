/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static bool	check_coders(t_shared *s)
{
	int		i;
	int		finished;
	long	now;

	i = 0;
	finished = 0;
	now = now_ms();
	while (i < s->config->coders_number)
	{
		if (s->coders[i].count_compile >= s->config->required_number)
			finished++;
		else if (now - s->coders[i].s_compile_time
			>= s->config->burnout_time)
		{
			log_locked(s, s->coders[i].id, "burned out");
			return (true);
		}
		i++;
	}
	return (finished == s->config->coders_number);
}

void	*monitor_routine(void *arg)
{
	t_shared	*s;

	s = arg;
	if (!await_start(s))
		return (NULL);
	pthread_mutex_lock(&s->mutex_stop);
	while (!s->stopped)
	{
		if (check_coders(s))
		{
			s->stopped = true;
			pthread_cond_broadcast(&s->changed);
			break ;
		}
		wait_change(s);
	}
	pthread_mutex_unlock(&s->mutex_stop);
	return (NULL);
}
