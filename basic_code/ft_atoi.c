/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_atoi.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	overflow(long out, char str, int check_minus)
{
	long	limit;

	limit = INT_MAX;
	if (check_minus == -1)
		limit = -(long)INT_MIN;
	return (out > limit / 10 || (out == limit / 10
			&& str - '0' > limit % 10));
}

int	ft_atoi(const char *str)
{
	int		i;
	int		sign;
	long	out;

	i = 0;
	sign = 1;
	out = 0;
	while (str[i] == ' ' || ('\t' <= str[i] && str[i] <= '\r'))
		i++;
	if (str[i] == '-' || str[i] == '+')
	{
		if (str[i] == '-')
			sign = -1;
		i++;
	}
	while ('0' <= str[i] && str[i] <= '9')
	{
		if (overflow(out, str[i], sign))
			return (-1);
		out = out * 10 + str[i++] - '0';
	}
	return ((int)(sign * out));
}
