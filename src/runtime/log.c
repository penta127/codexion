/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	log_locked(t_shared *shared, int id, const char *message)
{
	pthread_mutex_lock(&shared->mutex_log);
	printf("%ld %d %s\n", now_ms() - shared->start_time, id, message);
	pthread_mutex_unlock(&shared->mutex_log);
}

void	log_compile_locked(t_shared *shared, t_coder *coder)
{
	long	timestamp;

	pthread_mutex_lock(&shared->mutex_log);
	coder->s_compile_time = now_ms();
	timestamp = coder->s_compile_time - shared->start_time;
	printf("%ld %d has taken a dongle\n%ld %d has taken a dongle\n"
		"%ld %d is compiling\n", timestamp, coder->id,
		timestamp, coder->id, timestamp, coder->id);
	pthread_mutex_unlock(&shared->mutex_log);
}

bool	log_action(t_coder *coder, const char *message)
{
	t_shared	*shared;
	bool		running;

	shared = coder->shared;
	pthread_mutex_lock(&shared->mutex_stop);
	running = !shared->stopped;
	if (running)
		log_locked(shared, coder->id, message);
	pthread_mutex_unlock(&shared->mutex_stop);
	return (running);
}
