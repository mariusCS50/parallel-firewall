// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

void *consumer_thread(so_consumer_ctx_t *ctx)
{
  so_ring_buffer_t *ring = ctx->producer_rb;
  char buffer[PKT_SZ], out_buf[PKT_SZ];
  while (true) {
    pthread_mutex_lock(&(ring->mutex));
    while (ring->packets_left < ctx->num_consumers && !ring->stop) {
      pthread_cond_wait(&(ring->payload_available), &(ring->mutex));
    }
    pthread_cond_broadcast(&(ring->payload_available));
    if (ring->stop && ring->packets_left == 0) {
      pthread_mutex_unlock(&(ring->mutex));
      pthread_exit(0);
    }
    ring_buffer_dequeue(ring, buffer, PKT_SZ);

    pthread_mutex_unlock(&(ring->mutex));
    //sem_post(&(ring->buffer_free));

    struct so_packet_t *pkt = (struct so_packet_t *)buffer;
    int action = process_packet(pkt);
    unsigned long hash = packet_hash(pkt);
    unsigned long timestamp = pkt->hdr.timestamp;

    int len1 = snprintf(out_buf, 256, "%s %016lx %lu processed by thread %d\n", RES_TO_STR(action), hash, timestamp, ctx->thread_id);
		write(0, out_buf, len1);

    // int len = snprintf(out_buf, 256, "%s %016lx %lu\n", RES_TO_STR(action), hash, timestamp);
		// write(ctx->out_fd, out_buf, len);
  }
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
  // so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
  // ctx->producer_rb = rb;
  // ctx->num_consumers = num_consumers;
  // ctx->counter = 0;
  // ctx->out_fd = open(out_filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
  int out_fd = open(out_filename, O_RDWR|O_CREAT|O_TRUNC, 0666);

	for (int i = 0; i < num_consumers; i++) {
    so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
    ctx->producer_rb = rb;
    ctx->num_consumers = num_consumers;
    ctx->thread_id = i;
    ctx->out_fd = out_fd;
		pthread_create(&tids[i], NULL, &consumer_thread, (void *)ctx);
	}

	return num_consumers;
}
