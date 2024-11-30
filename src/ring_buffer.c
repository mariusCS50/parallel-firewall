// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "ring_buffer.h"
#include "packet.h"

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	ring->data = malloc(cap);
	ring->cap = cap;
	ring->write_pos = 0;
	ring->read_pos = 0;
	ring->stop = 0;
	ring->packets_left = 0;
	pthread_mutex_init(&(ring->thread_lock), NULL);
	pthread_cond_init(&(ring->buffer_available), NULL);
	pthread_cond_init(&(ring->payload_available), NULL);
	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	pthread_mutex_lock(&(ring->thread_lock));
	while (ring->packets_left == 1000)
		pthread_cond_wait(&ring->buffer_available, &ring->thread_lock);

	memcpy(ring->data + ring->write_pos, data, size);
	ring->write_pos = (ring->write_pos + size) % ring->cap;
	ring->packets_left = ring->packets_left + 1;

	pthread_cond_signal(&(ring->payload_available));
	pthread_mutex_unlock(&(ring->thread_lock));
	return 1;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	pthread_mutex_lock(&ring->thread_lock);
	while (!ring->stop && ring->packets_left == 0)
		pthread_cond_wait(&(ring->payload_available), &(ring->thread_lock));

	if (ring->stop && ring->packets_left == 0) {
		pthread_mutex_unlock(&(ring->thread_lock));
		return 0;
	}

	memcpy(data, ring->data + ring->read_pos, size);
	ring->read_pos = (ring->read_pos + size) % ring->cap;
	ring->packets_left = ring->packets_left - 1;

	pthread_cond_signal(&ring->buffer_available);
	pthread_mutex_unlock(&ring->thread_lock);
	return 1;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	pthread_mutex_destroy(&(ring->thread_lock));
	pthread_cond_destroy(&(ring->buffer_available));
	pthread_cond_destroy(&(ring->payload_available));
	free(ring->data);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	ring->stop = 1;
}
