#include "codexion.h"

void input(t_config *config, char **argv)
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
