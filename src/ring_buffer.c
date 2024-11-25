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
  sem_init(&(ring->buffer_free), 0, 10);
  pthread_mutex_init(&(ring->mutex), NULL);
  pthread_cond_init(&(ring->payload_available), NULL);
	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
  sem_wait(&(ring->buffer_free));
  pthread_mutex_lock(&(ring->mutex));
	memcpy(ring->data + ring->write_pos % ring->cap, data, size);
  ring->write_pos = ring->write_pos + size;
  ring->packets_left++;
  pthread_cond_signal(&(ring->payload_available));
  pthread_mutex_unlock(&(ring->mutex));
	return -1;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	memcpy(data, ring->data + ring->read_pos % ring->cap, size);
  ring->read_pos = ring->read_pos + size;
  ring->packets_left--;
	return -1;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
  sem_destroy(&(ring->buffer_free));
  pthread_mutex_destroy(&(ring->mutex));
  pthread_cond_destroy(&(ring->payload_available));
	free(ring->data);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
  pthread_mutex_lock(&(ring->mutex));
  ring->stop = 1;
  pthread_cond_broadcast(&(ring->payload_available));
  pthread_mutex_unlock(&(ring->mutex));
}
