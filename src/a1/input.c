/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	input(t_config *config, char **argv)
{
	config->coders_number = ft_atoi(argv[1]);
	config->burnout_time = ft_atoi(argv[2]);
	config->compile_time = ft_atoi(argv[3]);
	config->debug_time = ft_atoi(argv[4]);
	config->refactor_time = ft_atoi(argv[5]);
	config->required_number = ft_atoi(argv[6]);
	config->cooldown_dongle = ft_atoi(argv[7]);
	config->scheduler = argv[8];
}
