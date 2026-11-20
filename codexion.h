/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <stdio.h>
# include <stdlib.h>
# include <limits.h>
# include <stdbool.h>
# include <string.h>
# include <pthread.h>
# include <sys/time.h>
# include <time.h>
# include <unistd.h>

typedef struct s_shared	t_shared;

typedef struct s_config
{
	int		coders_number;
	int		burnout_time;
	int		compile_time;
	int		debug_time;
	int		refactor_time;
	int		required_number;
	int		cooldown_dongle;
	char	*scheduler;
}	t_config;

typedef struct s_coder
{
	int			id;
	pthread_t	thread;
	int			count_compile;
	long		s_compile_time;
	int			left_dongle_number;
	int			right_dongle_number;
	bool		granted;
	bool		queued;
	t_shared	*shared;
}	t_coder;

typedef struct s_dongle
{
	int				id;
	pthread_mutex_t	mutex;
	long			release_time;
	bool			in_use;
	bool			blocked;
}	t_dongle;

typedef struct s_request
{
	int				id;
	long			priority;
	int				tie_breaker;
	unsigned long	order;
}	t_request;

typedef struct s_heap
{
	t_request	*data;
	int			size;
	int			capacity;
}	t_heap;

struct s_shared
{
	pthread_mutex_t	mutex_log;
	bool			stopped;
	pthread_mutex_t	mutex_stop;
	pthread_cond_t	changed;
	bool			ready;
	long			start_time;
	unsigned long	next_order;
	const t_config	*config;
	t_coder			*coders;
	t_dongle		*dongles;
	t_heap			*heap;
	t_request		*pending;
	int				started_count;
	int				prepared_count;
	pthread_t		monitor;
	bool			monitor_started;
};

bool		error_check(int argc, char **argv);
bool		number_check(char argv[], int allow_zero);
void		input(t_config *config, char **argv);
bool		overflow(long out, char str, int check_minus);
int			ft_atoi(const char *str);
bool		simulation(t_config *config);
bool		init_shared(t_shared *shared);
t_coder		*init_coder(t_config *config);
t_dongle	*init_dongle(t_config *config);
t_heap		*init_heap(t_config *config);
bool		heap_push(t_heap *heap, t_request request);
bool		heap_pop(t_heap *heap, t_request *request);
void		free_heap(t_heap *heap);
bool		request_before(t_request a, t_request b);
void		destroy_dongles(t_dongle *dongles, int count);
void		cleanup_shared(t_shared *shared);
bool		start_coders(t_shared *shared);
bool		finish_join_coders(t_shared *shared);
void		*coder_routine(void *arg);
void		*monitor_routine(void *arg);
bool		await_start(t_shared *shared);
bool		prepare_coder_start(t_coder *coder);
long		now_ms(void);
void		wait_change(t_shared *shared);
bool		wait_until(t_shared *shared, long deadline);
void		log_locked(t_shared *shared, int id, const char *message);
void		log_compile_locked(t_shared *shared, t_coder *coder, long start);
bool		log_action(t_coder *coder, const char *message);
void		dispatch_requests(t_shared *shared);
bool		enqueue_request(t_coder *coder);
bool		acquire_dongles(t_coder *coder);
bool		finish_compile(t_coder *coder);
void		release_dongles(t_coder *coder);
bool		dongles_available(t_shared *s, t_coder *c, long now);
void		grant_dongles(t_shared *s, t_coder *c, long start);

#endif
