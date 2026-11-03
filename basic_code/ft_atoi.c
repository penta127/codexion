#include "codexion.h"

int	overflow(long out, char str, int check_minus)
{
	long	max_div;
	long	max_mod;
	long	min_div;
	long	min_mod;

	if (check_minus == 1)
	{
		max_div = INT_MAX / 10;
		max_mod = INT_MAX % 10;
		if ((out > max_div) || (out == max_div && ((int)str - '0') > max_mod))
			return (-1);
		return (0);
	}
	else
	{
		min_div = -(INT_MIN / 10);
		min_mod = -(INT_MIN % 10);
		if ((out > min_div) || (out == min_div && ((int)str - '0') > min_mod))
			return (-1);
		return (0);
	}
	return (0);
}

int	ft_atoi(const char *str)
{
	int	i;
	int	check_minus;
	long	out;

	i = 0;
	check_minus = 1;
	out = 0;
	while ((str[i] == ' ') || ('\t' <= str[i] && str[i] <= '\r'))
		i++;
	if (str[i] == '-' || str[i] == '+')
	{
		if (str[i] == '-')
			check_minus = check_minus * -1;
		i++;
	}
	while ('0' <= str[i] && str[i] <= '9')
	{
		if (overflow(out, str[i], check_minus) == -1)
			return (-1);
		out = (out * 10) + str[i] - '0';
		i++;
	}
	return (check_minus * out);
}