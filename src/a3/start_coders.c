/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   start_coders.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static bool	start_failed(t_shared *shared)
{
	pthread_mutex_lock(&shared->mutex_stop);
	shared->stopped = true;
	pthread_cond_broadcast(&shared->changed);
	pthread_mutex_unlock(&shared->mutex_stop);
	return (false);
}

bool	start_coders(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->config->coders_number)
	{
		if (pthread_create(&shared->coders[i].thread, NULL,
				coder_routine, &shared->coders[i]) != 0)
			return (start_failed(shared));
		shared->started_count++;
		i++;
	}
	if (pthread_create(&shared->monitor, NULL,
			monitor_routine, shared) != 0)
		return (start_failed(shared));
	shared->monitor_started = true;
	return (true);
}

bool	finish_join_coders(t_shared *shared)
{
	int		i;
	bool	success;

	success = true;
	i = 0;
	while (i < shared->started_count)
	{
		if (pthread_join(shared->coders[i].thread, NULL) != 0)
			success = false;
		i++;
	}
	if (shared->monitor_started
		&& pthread_join(shared->monitor, NULL) != 0)
		success = false;
	return (success);
}

bool	await_start(t_shared *shared)
{
	bool	running;

	pthread_mutex_lock(&shared->mutex_stop);
	while (!shared->ready && !shared->stopped)
		pthread_cond_wait(&shared->changed, &shared->mutex_stop);
	running = !shared->stopped;
	pthread_mutex_unlock(&shared->mutex_stop);
	return (running);
}
