/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shared.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	init_shared(t_shared *shared)
{
	shared->stopped = false;
	if (pthread_mutex_init(&shared->mutex_log, NULL) != 0)
		return (false);
	if (pthread_mutex_init(&shared->mutex_stop, NULL) != 0)
	{
		pthread_mutex_destroy(&shared->mutex_log);
		return (false);
	}
	if (pthread_cond_init(&shared->changed, NULL) != 0)
	{
		pthread_mutex_destroy(&shared->mutex_stop);
		pthread_mutex_destroy(&shared->mutex_log);
		return (false);
	}
	return (true);
}

void	cleanup_shared(t_shared *shared)
{
	free(shared->pending);
	free_heap(shared->heap);
	destroy_dongles(shared->dongles, shared->config->coders_number);
	free(shared->coders);
	pthread_cond_destroy(&shared->changed);
	pthread_mutex_destroy(&shared->mutex_stop);
	pthread_mutex_destroy(&shared->mutex_log);
}
