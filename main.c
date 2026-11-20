/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int argc, char **argv)
{
	t_config	config;

	if (!error_check(argc, argv))
	{
		fprintf(stderr, "Usage: ./codexion coders burnout compile debug "
			"refactor required cooldown fifo|edf\n");
		return (1);
	}
	input(&config, argv);
	if (!simulation(&config))
	{
		fprintf(stderr, "Error: simulation initialization or thread failure\n");
		return (1);
	}
	return (0);
}
