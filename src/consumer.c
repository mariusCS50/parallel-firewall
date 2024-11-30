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

packet_entry_t log_buf[4];
size_t curr_log_index;

int compare_by_timestamp(const void *a, const void *b)
{
		const packet_entry_t *entry_a = (const packet_entry_t *)a;
		const packet_entry_t *entry_b = (const packet_entry_t *)b;

		return entry_a->timestamp - entry_b->timestamp;
}

void consumer_thread(so_consumer_ctx_t *ctx)
{
	so_ring_buffer_t *ring = ctx->producer_rb;
	char buffer[PKT_SZ], out_buf[PKT_SZ];

	while (true) {
		if (ring_buffer_dequeue(ring, buffer, PKT_SZ) == 0)
			break;

		struct so_packet_t *pkt = (struct so_packet_t *)buffer;
		int action = process_packet(pkt);
		unsigned long hash = packet_hash(pkt);
		unsigned long timestamp = pkt->hdr.timestamp;

		pthread_mutex_lock(&ring->thread_lock);
		log_buf[curr_log_index].action = action;
		log_buf[curr_log_index].hash = hash;
		log_buf[curr_log_index].timestamp = timestamp;
		curr_log_index++;

		if (ctx->bypass_barrier && curr_log_index < ctx->num_threads) {
			pthread_mutex_unlock(&ring->thread_lock);
			continue;
		}

		if (curr_log_index == ctx->num_threads) {
			qsort(log_buf, ctx->num_threads, sizeof(packet_entry_t), compare_by_timestamp);

			for (size_t i = 0; i < ctx->num_threads; i++) {
				int len = snprintf(out_buf, 256, "%s %016lx %lu\n",
													 RES_TO_STR(log_buf[i].action),
													 log_buf[i].hash,
													 log_buf[i].timestamp);
				write(ctx->out_fd, out_buf, len);
			}

			if (ctx->bypass_barrier) {
				pthread_mutex_unlock(&ring->thread_lock);
				continue;
			}

			if (ring->stop && ring->packets_left < ctx->num_threads) {
				ctx->num_threads = ring->packets_left;
				ctx->bypass_barrier = 1;
			}

			curr_log_index = 0;
		}
		pthread_mutex_unlock(&ring->thread_lock);
		pthread_barrier_wait(&ctx->wait_for_all_threads);
	}
}

int create_consumers(pthread_t *tids,
										 int num_consumers,
										 struct so_ring_buffer_t *rb,
										 const char *out_filename)
{
	so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));

	ctx->producer_rb = rb;
	ctx->bypass_barrier = 0;
	ctx->num_threads = num_consumers;
	ctx->out_fd = open(out_filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
	pthread_barrier_init(&ctx->wait_for_all_threads, NULL, num_consumers);

	curr_log_index = 0;

	for (int i = 0; i < num_consumers; i++)
		pthread_create(&tids[i], NULL, (void *)consumer_thread, (void *)ctx);

	return num_consumers;
}
