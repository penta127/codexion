/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_deadlines.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <assert.h>

long	__wrap_now_ms(void)
{
	static int	calls;

	if (calls++ == 0)
		return (999);
	return (1001);
}

static void	setup(t_shared *s, t_config *config)
{
	memset(config, 0, sizeof(*config));
	memset(s, 0, sizeof(*s));
	config->coders_number = 2;
	config->burnout_time = 120;
	config->required_number = 1;
	config->scheduler = "edf";
	s->config = config;
	assert(init_shared(s));
	s->coders = init_coder(config);
	s->dongles = init_dongle(config);
	s->heap = init_heap(config);
	s->pending = malloc(sizeof(t_request) * 2);
	assert(s->coders && s->dongles && s->heap && s->pending);
	s->coders[0].shared = s;
	s->coders[1].shared = s;
	s->start_time = 0;
}

static void	test_expired(t_shared *s)
{
	s->coders[0].s_compile_time = 881;
	pthread_mutex_lock(&s->mutex_stop);
	assert(enqueue_request(&s->coders[0]));
	dispatch_requests(s);
	assert(!s->coders[0].granted);
	pthread_mutex_unlock(&s->mutex_stop);
	assert(finish_compile(&s->coders[0]));
	assert(s->coders[0].count_compile == 0);
	s->coders[0].s_compile_time = 880;
	assert(finish_compile(&s->coders[0]));
	assert(s->coders[0].count_compile == 0);
}

static void	test_valid(t_shared *s)
{
	s->coders[1].s_compile_time = 882;
	assert(finish_compile(&s->coders[1]));
	assert(s->coders[1].count_compile == 1);
	pthread_mutex_lock(&s->mutex_stop);
	log_compile_locked(s, &s->coders[1], 999);
	assert(s->coders[1].s_compile_time == 999);
	pthread_mutex_unlock(&s->mutex_stop);
}

int	main(void)
{
	t_shared	s;
	t_config	config;

	setup(&s, &config);
	s.coders[0].s_compile_time = 880;
	pthread_mutex_lock(&s.mutex_stop);
	assert(enqueue_request(&s.coders[0]));
	dispatch_requests(&s);
	assert(s.coders[0].granted && s.coders[0].s_compile_time == 999);
	release_dongles(&s.coders[0]);
	pthread_mutex_unlock(&s.mutex_stop);
	test_expired(&s);
	test_valid(&s);
	cleanup_shared(&s);
	printf("PASS: deadline boundaries and validated compile timestamp\n");
	return (0);
}
