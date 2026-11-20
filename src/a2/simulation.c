/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static bool	prepare_data(t_shared *shared, t_config *config)
{
	int	i;

	shared->config = config;
	shared->coders = init_coder(config);
	shared->dongles = init_dongle(config);
	shared->heap = init_heap(config);
	shared->pending = malloc(sizeof(t_request) * config->coders_number);
	if (!shared->coders || !shared->dongles
		|| !shared->heap || !shared->pending)
		return (false);
	i = 0;
	while (i < config->coders_number)
	{
		shared->coders[i].shared = shared;
		i++;
	}
	return (true);
}

static void	set_start_time(t_shared *shared)
{
	int	i;

	shared->start_time = now_ms();
	i = 0;
	while (i < shared->config->coders_number)
	{
		shared->coders[i].s_compile_time = shared->start_time;
		if (strcmp(shared->config->scheduler, "edf") == 0)
			shared->heap->data[i].priority += shared->start_time;
		i++;
	}
	shared->ready = true;
}

static bool	begin_simulation(t_shared *shared)
{
	bool	running;

	pthread_mutex_lock(&shared->mutex_stop);
	while (!shared->stopped
		&& shared->prepared_count < shared->config->coders_number)
		pthread_cond_wait(&shared->changed, &shared->mutex_stop);
	if (!shared->stopped)
		set_start_time(shared);
	running = !shared->stopped;
	pthread_cond_broadcast(&shared->changed);
	pthread_mutex_unlock(&shared->mutex_stop);
	return (running);
}

bool	simulation(t_config *config)
{
	t_shared	shared;
	bool		success;

	memset(&shared, 0, sizeof(shared));
	if (!init_shared(&shared))
		return (false);
	if (!prepare_data(&shared, config))
	{
		cleanup_shared(&shared);
		return (false);
	}
	success = true;
	if (config->required_number > 0)
	{
		success = start_coders(&shared);
		if (success)
			success = begin_simulation(&shared);
		if (!finish_join_coders(&shared))
			success = false;
	}
	cleanup_shared(&shared);
	return (success);
}
