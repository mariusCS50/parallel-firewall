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

typedef struct processed_data {
  int action;
  unsigned long hash;
  unsigned long timestamp;
} processed_data_t;

processed_data_t log_buf[5];
int id = 0;

int comp(const void *a, const void *b) {
    int timestamp1 = (*(processed_data_t *)a).timestamp;
    int timestamp2 = (*(processed_data_t *)b).timestamp;
    return timestamp1 - timestamp2;
}

void *consumer_thread(so_consumer_ctx_t *ctx)
{
  so_ring_buffer_t *ring = ctx->producer_rb;
  char buffer[PKT_SZ], out_buf[PKT_SZ];
  while (true) {
    pthread_mutex_lock(&(ring->mutex));
    while (ring->packets_left == 0 && !ring->stop) {
      pthread_cond_wait(&(ring->payload_available), &(ring->mutex));
    }
    if (ring->stop && ring->packets_left == 0) {
      pthread_mutex_unlock(&(ring->mutex));
      pthread_exit(0);
    }
    ring_buffer_dequeue(ring, buffer, PKT_SZ);

    pthread_mutex_unlock(&(ring->mutex));
    sem_post(&(ring->buffer_free));

    struct so_packet_t *pkt = (struct so_packet_t *)buffer;
    int action = process_packet(pkt);
    unsigned long hash = packet_hash(pkt);
    unsigned long timestamp = pkt->hdr.timestamp;

    pthread_mutex_lock(&(ring->mutex));
    log_buf[id].action = action;
    log_buf[id].hash = hash;
    log_buf[id].timestamp = timestamp;
    id++;
    if (id == 5) {
      qsort(log_buf, 5, sizeof(processed_data_t), comp);
      for (int i = 0; i < 5; i++) {
        int len = snprintf(out_buf, 256, "%s %016lx %lu\n", RES_TO_STR(log_buf[i].action), log_buf[i].hash, log_buf[i].timestamp);
		    write(ctx->out_fd, out_buf, len);
      }
      id = 0;
    }
    pthread_mutex_unlock(&(ring->mutex));
  }
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
  so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
  ctx->producer_rb = rb;
  ctx->num_consumers = num_consumers;
  ctx->out_fd = open(out_filename, O_RDWR|O_CREAT|O_TRUNC, 0666);

	for (int i = 0; i < num_consumers; i++) {
		pthread_create(&tids[i], NULL, &consumer_thread, (void *)ctx);
	}

	return num_consumers;
}
