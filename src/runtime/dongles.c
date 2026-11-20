/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	lock_pair(t_dongle *left, t_dongle *right)
{
	if (left->id < right->id)
	{
		pthread_mutex_lock(&left->mutex);
		pthread_mutex_lock(&right->mutex);
	}
	else
	{
		pthread_mutex_lock(&right->mutex);
		pthread_mutex_lock(&left->mutex);
	}
}

static void	unlock_pair(t_dongle *left, t_dongle *right)
{
	if (left->id < right->id)
	{
		pthread_mutex_unlock(&right->mutex);
		pthread_mutex_unlock(&left->mutex);
	}
	else
	{
		pthread_mutex_unlock(&left->mutex);
		pthread_mutex_unlock(&right->mutex);
	}
}

bool	dongles_available(t_shared *s, t_coder *c, long now)
{
	t_dongle	*left;
	t_dongle	*right;
	bool		available;

	if (c->left_dongle_number == c->right_dongle_number)
		return (false);
	left = &s->dongles[c->left_dongle_number];
	right = &s->dongles[c->right_dongle_number];
	lock_pair(left, right);
	available = !left->in_use && !right->in_use
		&& !left->blocked && !right->blocked
		&& left->release_time <= now && right->release_time <= now;
	unlock_pair(left, right);
	return (available);
}

void	grant_dongles(t_shared *s, t_coder *c)
{
	t_dongle	*left;
	t_dongle	*right;

	left = &s->dongles[c->left_dongle_number];
	right = &s->dongles[c->right_dongle_number];
	lock_pair(left, right);
	left->in_use = true;
	right->in_use = true;
	unlock_pair(left, right);
	c->granted = true;
	log_compile_locked(s, c);
}

void	release_dongles(t_coder *coder)
{
	t_shared	*s;
	t_dongle	*left;
	t_dongle	*right;
	long		available_at;

	s = coder->shared;
	left = &s->dongles[coder->left_dongle_number];
	right = &s->dongles[coder->right_dongle_number];
	available_at = now_ms() + s->config->cooldown_dongle;
	lock_pair(left, right);
	left->in_use = false;
	right->in_use = false;
	left->release_time = available_at;
	right->release_time = available_at;
	unlock_pair(left, right);
	coder->granted = false;
}
