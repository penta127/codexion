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
	int parent;
	t_request tmp;

	if (heap == NULL || heap->data == NULL)
		return (-1);
	if (heap->capacity <= heap->size)
		return (-1);
	i = heap->size;
	heap->data[i] = request;
	heap->size++;
	while(i > 0)
	{
		parent = (i - 1) / 2;
		if (heap->data[parent].priority <= heap->data[i].priority)
			break;
		tmp = heap->data[parent];
		heap->data[parent] = heap->data[i];
		heap->data[i] = tmp;
		i = parent;
	}
	return(0);
}


int heap_pop(t_heap *heap, t_request *request)
{
	int i;
	int left;
	int right;
	int smallest;
	t_request tmp;

	if (heap == NULL || heap->data == NULL || request == NULL || heap->size <= 0)
		return (-1);
	*request = heap->data[0];
	heap->data[0] = heap->data[heap->size - 1];
	heap->size--;
	i = 0;
	while (1)
	{
		left = 2 * i + 1;
		right = 2 * i + 2;
		smallest = i;
		if (left < heap->size
			&& heap->data[left].priority < heap->data[smallest].priority)
			smallest = left;
		if (right < heap->size
			&& heap->data[right].priority < heap->data[smallest].priority)
			smallest = right;
		if (smallest == i)
			break ;
		tmp = heap->data[i];
		heap->data[i] = heap->data[smallest];
		heap->data[smallest] = tmp;
		i = smallest;
	}
	return (0);
}

void free_heap(t_heap *heap)
{
	if (heap == NULL)
		return;
	free(heap->data);
	free(heap);
	return;
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
