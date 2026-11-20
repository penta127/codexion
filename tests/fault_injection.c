/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   fault_injection.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koito <koito@student.42tokyo.jp>           #+#  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/20 00:00:00 by koito             #+#    #+#             */
/*   Updated: 2026/11/20 00:00:00 by koito            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <errno.h>

void	*__real_malloc(size_t size);
int		__real_pthread_mutex_init(pthread_mutex_t *m,
			const pthread_mutexattr_t *attr);
int		__real_pthread_cond_init(pthread_cond_t *c,
			const pthread_condattr_t *attr);
int		__real_pthread_create(pthread_t *t, const pthread_attr_t *attr,
			void *(*routine)(void *), void *arg);

void	*__wrap_malloc(size_t size)
{
	static int	calls;
	char		*fail;

	fail = getenv("FAIL_MALLOC");
	if (fail && ++calls == atoi(fail))
		return (NULL);
	return (__real_malloc(size));
}

int	__wrap_pthread_mutex_init(pthread_mutex_t *m,
	const pthread_mutexattr_t *attr)
{
	static int	calls;
	char		*fail;

	fail = getenv("FAIL_MUTEX");
	if (fail && ++calls == atoi(fail))
		return (ENOMEM);
	return (__real_pthread_mutex_init(m, attr));
}

int	__wrap_pthread_cond_init(pthread_cond_t *c,
	const pthread_condattr_t *attr)
{
	if (getenv("FAIL_COND"))
		return (ENOMEM);
	return (__real_pthread_cond_init(c, attr));
}

int	__wrap_pthread_create(pthread_t *t, const pthread_attr_t *attr,
	void *(*routine)(void *), void *arg)
{
	static int	calls;
	char		*fail;

	fail = getenv("FAIL_THREAD");
	if (fail && ++calls == atoi(fail))
		return (EAGAIN);
	return (__real_pthread_create(t, attr, routine, arg));
}
