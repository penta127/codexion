/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error_check.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	number_check(char argv[], int allow_zero)
{
	int	num;
	int	i;

	i = 0;
	if (argv[0] == '\0')
		return (false);
	while (argv[i] != '\0')
	{
		if (!('0' <= argv[i] && argv[i] <= '9'))
			return (false);
		i++;
	}
	num = ft_atoi(argv);
	if (num < 0 || (!allow_zero && num == 0))
		return (false);
	return (true);
}

bool	error_check(int argc, char **argv)
{
	int	i;

	if (argc != 9)
		return (false);
	i = 1;
	while (i <= 7)
	{
		if (!number_check(argv[i], i != 1))
			return (false);
		i++;
	}
	if (strcmp(argv[8], "fifo") != 0 && strcmp(argv[8], "edf") != 0)
		return (false);
	return (true);
}
