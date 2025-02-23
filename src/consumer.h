/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include <pthread.h>
#include <semaphore.h>
#include "ring_buffer.h"
#include "packet.h"

typedef struct so_consumer_ctx_t {
	struct so_ring_buffer_t *producer_rb;

	size_t out_fd;
	size_t num_threads;
	size_t bypass_barrier;

	pthread_barrier_t wait_for_all_threads;
} so_consumer_ctx_t;

int create_consumers(pthread_t *tids,
										 int num_consumers,
										 so_ring_buffer_t *rb,
										 const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
