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

typedef struct packet_entry {
    int action;
    unsigned long hash;
    unsigned long timestamp;
} packet_entry_t;

packet_entry_t log[5];
int id = 0;

int compare_by_timestamp(const void *a, const void *b) {
    const packet_entry_t *entry_a = (const packet_entry_t *)a;
    const packet_entry_t *entry_b = (const packet_entry_t *)b;

    return entry_a->timestamp - entry_b->timestamp;
}


void *consumer_thread(so_consumer_ctx_t *ctx)
{
  so_ring_buffer_t *ring = ctx->producer_rb;
  char buffer[PKT_SZ], out_buf[PKT_SZ];
  while (true) {
    if (ring_buffer_dequeue(ring, buffer, PKT_SZ) == 0) {
      //close(ctx->out_fd);
      break;
    }

    struct so_packet_t *pkt = (struct so_packet_t *)buffer;
    int action = process_packet(pkt);
    unsigned long hash = packet_hash(pkt);
    unsigned long timestamp = pkt->hdr.timestamp;

    pthread_mutex_lock(&(ring->mutex));
    log[id].action = action;
    log[id].hash = hash;
    log[id].timestamp = timestamp;
    id++;
    if (id == 5) {
      qsort(log, 5, sizeof(packet_entry_t), compare_by_timestamp);
      for (int i = 0; i < 5; i++) {
        int len = snprintf(out_buf, 256, "%s %016lx %lu\n", RES_TO_STR(log[i].action), log[i].hash, log[i].timestamp);
        write(ctx->out_fd, out_buf, len);
		    //write(0, "LINE\n", 5);
      }
      id = 0;
    }
    // int len = snprintf(out_buf, 256, "%s %016lx %lu\n", RES_TO_STR(action), hash, timestamp);
    // write(ctx->out_fd, out_buf, len);
		//write(0, "LINE\n", 5);
    pthread_mutex_unlock(&(ring->mutex));
  }
  return;
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
