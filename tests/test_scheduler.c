/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_scheduler.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <assert.h>

static void	setup(t_shared *s, t_config *config, char *policy)
{
	int	i;

	memset(config, 0, sizeof(*config));
	memset(s, 0, sizeof(*s));
	config->coders_number = 6;
	config->burnout_time = 3000;
	config->scheduler = policy;
	s->config = config;
	assert(init_shared(s));
	s->coders = init_coder(config);
	s->dongles = init_dongle(config);
	s->heap = init_heap(config);
	s->pending = malloc(sizeof(t_request) * 6);
	assert(s->coders && s->dongles && s->heap && s->pending);
	s->start_time = now_ms();
	i = -1;
	while (++i < 6)
	{
		s->coders[i].shared = s;
		s->coders[i].s_compile_time = s->start_time;
	}
}

static void	test_fifo(void)
{
	t_shared	s;
	t_config	config;

	setup(&s, &config, "fifo");
	s.dongles[0].in_use = true;
	s.dongles[5].in_use = true;
	pthread_mutex_lock(&s.mutex_stop);
	assert(enqueue_request(&s.coders[0]));
	assert(enqueue_request(&s.coders[1]));
	assert(enqueue_request(&s.coders[3]));
	dispatch_requests(&s);
	assert(!s.coders[0].granted && !s.coders[1].granted);
	assert(s.coders[3].granted);
	s.dongles[0].in_use = false;
	s.dongles[5].in_use = false;
	dispatch_requests(&s);
	assert(s.coders[0].granted && !s.coders[1].granted);
	release_dongles(&s.coders[0]);
	dispatch_requests(&s);
	assert(s.coders[1].granted);
	pthread_mutex_unlock(&s.mutex_stop);
	cleanup_shared(&s);
}

static void	test_edf(void)
{
	t_shared	s;
	t_config	config;
	t_request	request;

	setup(&s, &config, "edf");
	pthread_mutex_lock(&s.mutex_stop);
	assert(enqueue_request(&s.coders[0]));
	assert(enqueue_request(&s.coders[1]));
	dispatch_requests(&s);
	assert(!s.coders[0].granted && s.coders[1].granted);
	release_dongles(&s.coders[1]);
	dispatch_requests(&s);
	assert(s.coders[0].granted);
	s.coders[5].s_compile_time = s.start_time + 200;
	s.coders[0].s_compile_time = s.start_time + 100;
	assert(enqueue_request(&s.coders[5]));
	assert(enqueue_request(&s.coders[0]));
	assert(heap_pop(s.heap, &request) && request.id == 1);
	assert(heap_pop(s.heap, &request) && request.id == 6);
	pthread_mutex_unlock(&s.mutex_stop);
	cleanup_shared(&s);
}

static void	test_eligible_edf(void)
{
	t_shared	s;
	t_config	config;

	setup(&s, &config, "edf");
	pthread_mutex_lock(&s.mutex_stop);
	s.dongles[0].in_use = true;
	s.dongles[5].in_use = true;
	s.coders[0].s_compile_time -= 100;
	assert(enqueue_request(&s.coders[0]));
	assert(enqueue_request(&s.coders[1]));
	dispatch_requests(&s);
	assert(!s.coders[0].granted && s.coders[1].granted);
	assert(s.heap->size == 1 && s.heap->data[0].id == 1);
	pthread_mutex_unlock(&s.mutex_stop);
	cleanup_shared(&s);
}

int	main(void)
{
	test_fifo();
	test_edf();
	test_eligible_edf();
	printf("PASS: FIFO reservation, independent pairs and EDF tie-break\n");
	return (0);
}
