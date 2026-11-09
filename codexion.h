#ifndef CODEXION_H
# define CODEXION_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>


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

typedef struct s_coder
{
    int id;
    pthread_t thread;
    int count_compile;
    long s_compile_time;
    int left_dongle_number;
    int right_dongle_number;
}t_coder;

typedef struct s_dongle
{
    int id;
    pthread_mutex_t mutex;
    long release_time;
}t_dongle;

typedef struct s_request
{
	int id;
	long priority;
}t_request;

typedef struct s_heap
{
	t_request *data;
	int size;
	int capacity;
}t_heap;

//input a_1
int error_check(int argc, char **argv);
int number_check(char argv[], int yes_no_zero);
void input(t_config *config, char **argv);

//a2
int simulation(t_config *config);
t_dongle *init_dongle(t_config *config);
t_coder *init_coder(t_config *config);



//ft_atoi
int	overflow(long out, char str, int check_minus);
int	ft_atoi(const char *str);


#endif
