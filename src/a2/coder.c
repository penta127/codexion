#include "codexion.h"

t_coder *init_coder(t_config *config)
{
	t_coder *coders;
	int i;

	coders = malloc(sizeof(t_coder) * config->coders_number);
	if(coders == NULL)
		return (NULL);
	i = 0;
	while(i < config->coders_number)
	{
		coders[i].id = i + 1;
		coders[i].count_compile = 0;
		coders[i].left_dongle_number = i;
		if (i != config->coders_number - 1)
			coders[i].right_dongle_number = i + 1;
		else
			coders[i].right_dongle_number = 0;
		i++;
	}
	return (coders);
}


t_dongle *init_dongle(t_config *config)
{
    t_dongle *dongle;
    int check;
    int i;

    dongle = malloc(sizeof(t_dongle) * config->coders_number);
    if (dongle == NULL)
        return NULL;
    i = 0;
    while (i < config->coders_number)
    {
        dongle[i].id = i + 1;
        dongle[i].release_time = -1;
        check = pthread_mutex_init(&dongle[i].mutex, NULL);
        if (check != 0)
        {
            i--;
            while(0 <= i)
            {
                pthread_mutex_destroy(&dongle[i].mutex);
                i--;
            }
            free(dongle);
            return(NULL);
        }
        i++;
    }
    return (dongle);
}


t_heap *init_heap(t_config *config)
{
	t_heap *heap;

	heap = malloc(sizeof(t_heap));
	if (heap == NULL)
		return (NULL);
	heap->data = malloc(sizeof(t_request) * config->coders_number);
	if (heap->data == NULL)
	{
		free(heap->data);
		free(heap);
		return(NULL);
	}
	heap->size = 0;
	heap->capacity = config->coders_number;
	return (heap);
}


int heap_push(t_heap *heap, t_request request)
{
	int i;

	i = 0;
	if (heap->capacity <= heap->size)
		return (-1);
	i = heap->size;
	heap->data[i] = request;
	heap->size++;



}


int simulation(t_config *config)
{
    t_coder *coders;
    t_dongle *dongle;
	t_heap *heap;

    coders = init_coder(config);
    if (coders == NULL)
        return(-1);
    dongle = init_dongle(config);
    if (dongle == NULL)
    {
        free(coders);
        return(-1);
    }
	heap =init_heap(config);
	if (heap == NULL)
	{
		free(coders);
		free(dongle);
		return(-1);
	}
}
