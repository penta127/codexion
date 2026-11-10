#include "codexion.h"

void *coder_routine(void *arg)
{
	t_coder *coder;
	coder = arg;
	return (NULL);
}

bool finish_join_coders(t_coder *coders, t_config *config)
{
	int i;

	i = 0;
	while(i < config->coders_number)
	{
		if (pthread_join(coders[i].thread, NULL) != 0)
			return (false);
		i++;
	}
	return (true);
}


bool start_coders(t_coder *coders, t_config *config)
{
	int i;
	int check;

	i = 0;
	while(i < config->coders_number)
	{
		check = pthread_create(&coders[i].thread, NULL, coder_routine, &coders[i]);
		if (check != 0)
		{
			i = i - 1;
			while(0 <= i)
			{
				pthread_join(coders[i].thread, NULL);
				i--;
			}
			return (false);
		}
		i++;
	}
	return (true);
}
