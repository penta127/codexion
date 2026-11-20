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

static void	begin_simulation(t_shared *shared)
{
	int	i;

	pthread_mutex_lock(&shared->mutex_stop);
	shared->start_time = now_ms();
	i = 0;
	while (i < shared->config->coders_number)
	{
		shared->coders[i].s_compile_time = shared->start_time;
		i++;
	}
	shared->ready = true;
	pthread_cond_broadcast(&shared->changed);
	pthread_mutex_unlock(&shared->mutex_stop);
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
			begin_simulation(&shared);
		if (!finish_join_coders(&shared))
			success = false;
	}
	cleanup_shared(&shared);
	return (success);
}
