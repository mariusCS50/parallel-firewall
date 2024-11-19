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

void consumer_thread(so_consumer_ctx_t *ctx)
{
  so_ring_buffer_t *ring = ctx->producer_rb;
  while (true) {
    char buffer[PKT_SZ], out_buf[PKT_SZ];
    //pthread_cond_wait(&(ring->packetAvailable), &(ring->mutex));
    //sem_wait(&(ring->packetReady));
    pthread_mutex_lock(&(ring->mutex));
    if (ring->stop && ring->read_pos == ring->write_pos) {
      pthread_mutex_unlock(&(ring->mutex));
      break;
    }
    while (!ring->stop && ring->read_pos == ring->write_pos) {
      pthread_cond_wait(&(ring->packetAvailable), &(ring->mutex));
    }
    ring_buffer_dequeue(ring, buffer, PKT_SZ);
    pthread_mutex_unlock(&(ring->mutex));
    sem_post(&(ring->bufferAvailable));

    struct so_packet_t *pkt = (struct so_packet_t *)buffer;
    int action = process_packet(pkt);
    unsigned long hash = packet_hash(pkt);
    unsigned long timestamp = pkt->hdr.timestamp;

    int len = snprintf(out_buf, 256, "%s %016lx %lu\n", RES_TO_STR(action), hash, timestamp);
		write(ctx->out_fd, out_buf, len);
  }
  pthread_mutex_lock(&(ring->mutex));
  ring->active_threads--;
  pthread_mutex_unlock(&(ring->mutex));
}


int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
  so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
  ctx->producer_rb = rb;
  ctx->producer_rb->active_threads = num_consumers;
  ctx->out_fd = open(out_filename, O_RDWR|O_CREAT|O_TRUNC, 0666);

	for (int i = 0; i < num_consumers; i++) {
		pthread_create(&tids[i], NULL, consumer_thread, ctx);
	}

  for (int i = 0; i < num_consumers; i++) {
    pthread_detach(tids[i]);
  }
  //pthread_detach(tids[0]);

	return num_consumers;
}
