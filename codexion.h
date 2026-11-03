#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

typedef struct s_config
{
    int coders_number;
    int burnout_time;
    int compile_time;
    int debug_time;
    int refactor_time;
    int required_number;
    int cooldown_dongle;
    char *scheduler;

}t_config;

//input a_1
int error_check(int argc, char **argv);
int number_check(char argv[], int yes_no_zero);
void input(t_config *config, char **argv);

//ft_atoi
int	overflow(long out, char str, int check_minus);
int	ft_atoi(const char *str);
