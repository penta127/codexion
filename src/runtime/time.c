/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long	now_ms(void)
{
	struct timespec	now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1000L + now.tv_nsec / 1000000L);
}

void	wait_change(t_shared *shared)
{
	pthread_mutex_unlock(&shared->mutex_stop);
	usleep(500);
	pthread_mutex_lock(&shared->mutex_stop);
}

bool	wait_until(t_shared *shared, long deadline)
{
	bool	running;

	pthread_mutex_lock(&shared->mutex_stop);
	while (!shared->stopped && now_ms() < deadline)
		wait_change(shared);
	running = !shared->stopped;
	pthread_mutex_unlock(&shared->mutex_stop);
	return (running);
}
