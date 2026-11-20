/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

t_coder	*init_coder(t_config *config)
{
	t_coder	*coders;
	int		i;

	coders = malloc(sizeof(t_coder) * config->coders_number);
	if (coders == NULL)
		return (NULL);
	memset(coders, 0, sizeof(t_coder) * config->coders_number);
	i = 0;
	while (i < config->coders_number)
	{
		coders[i].id = i + 1;
		coders[i].left_dongle_number = i;
		coders[i].right_dongle_number = (i + 1) % config->coders_number;
		i++;
	}
	return (coders);
}

void	destroy_dongles(t_dongle *dongles, int count)
{
	int	i;

	i = 0;
	if (!dongles)
		return ;
	while (i < count)
	{
		pthread_mutex_destroy(&dongles[i].mutex);
		i++;
	}
	free(dongles);
}

t_dongle	*init_dongle(t_config *config)
{
	t_dongle	*dongles;
	int			i;

	dongles = malloc(sizeof(t_dongle) * config->coders_number);
	if (dongles == NULL)
		return (NULL);
	memset(dongles, 0, sizeof(t_dongle) * config->coders_number);
	i = 0;
	while (i < config->coders_number)
	{
		dongles[i].id = i + 1;
		if (pthread_mutex_init(&dongles[i].mutex, NULL) != 0)
		{
			destroy_dongles(dongles, i);
			return (NULL);
		}
		i++;
	}
	return (dongles);
}
