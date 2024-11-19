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
  ring->len = 0;
  ring->stop = 0;
  ring->active_threads = 0;
  sem_init(&(ring->bufferAvailable), 0, 1000);
  //sem_init(&(ring->packetReady), 0, 0);
  pthread_mutex_init(&(ring->mutex), NULL);
  pthread_cond_init(&(ring->packetAvailable), NULL);
	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
  sem_wait(&(ring->bufferAvailable));
	memcpy(ring->data + ring->write_pos % ring->cap, data, size);
  ring->write_pos = ring->write_pos + size;
  pthread_mutex_lock(&(ring->mutex));
  pthread_cond_broadcast(&(ring->packetAvailable));
  pthread_mutex_unlock(&(ring->mutex));
  //sem_post(&(ring->packetReady));
  //pthread_cond_broadcast(&(ring->packetAvailable));
	return -1;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	memcpy(data, ring->data + ring->read_pos % ring->cap, size);
  ring->read_pos = ring->read_pos + size;
	return -1;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
  sem_destroy(&(ring->bufferAvailable));
  pthread_mutex_destroy(&(ring->mutex));
  pthread_cond_destroy(&(ring->packetAvailable));
  pthread_cond_destroy(&(ring->workingThreadsDone));
	free(ring->data);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
  ring->stop = 1;
}
