#include "parallel_queue.h"
#include "cached_parallel_queue.h"
#include "thread.h"
#include "time.h"

#include <stdio.h>
#include <stdlib.h>

#define MSG_TOTAL 1000000
#define QUEUE_CAPACITY 1 << 20

typedef struct BigStruct BigStruct;
struct BigStruct {
	u64 sequence;
	char padding[192];
};

void produce_messages_pq(void *pq) {
	ParallelQueue *q = (ParallelQueue *)pq;

	u64 counter = 0;
	while (counter != MSG_TOTAL) {
		BigStruct big = {0};
		big.sequence = counter;

		bool produced = parallel_queue_produce(q, &big);
		if (produced) {
			counter++;
		}
	}
}

void consume_messages_pq(void *pq) {
	ParallelQueue *q = (ParallelQueue *)pq;

	u64 counter = 0;
	while (counter != MSG_TOTAL) {
		BigStruct big;

		bool consumed = parallel_queue_consume(q, &big);

		if (consumed) {
			if (counter != big.sequence) {
				fprintf(stderr, "Sequence doesn't match count!"); 
				abort();
			}
			counter++;
		}
	}
}

void produce_two_step_messages_cq(void *cq) {
	CachedParallelQueue *q = (CachedParallelQueue *)cq;

	u64 counter = 0;
	while (counter != MSG_TOTAL) {

		BigStruct *seq_slot = cached_parallel_queue_produce_reserve(q);

		if (seq_slot) {
			*seq_slot = (BigStruct){0};
			seq_slot->sequence = counter;
			cached_parallel_queue_produce_commit(q);
			counter++;
		}
	}
}

void consume_two_step_messages_cq(void *cq) {
	CachedParallelQueue *q = (CachedParallelQueue *)cq;

	u64 counter = 0;
	while (counter != MSG_TOTAL) {
		const BigStruct *big = cached_parallel_queue_consume_reserve(q);

		if (big) {
			if (counter != big->sequence) {
				fprintf(stderr, "Sequence doesn't match count!"); 
				abort();
			}

			cached_parallel_queue_consume_commit(q);
			counter++;
		}
	}
}

void produce_messages_cq(void *cq) {
	CachedParallelQueue *q = (CachedParallelQueue *)cq;

	u64 counter = 0;
	while (counter != MSG_TOTAL) {
		BigStruct big = {0};
		big.sequence = counter;

		bool produced = cached_parallel_queue_produce(q, &big);
		if (produced) {
			counter++;
		}
	}
}

void consume_messages_cq(void *cq) {
	CachedParallelQueue *q = (CachedParallelQueue *)cq;

	u64 counter = 0;
	while (counter != MSG_TOTAL) {
		BigStruct big;

		bool consumed = cached_parallel_queue_consume(q, &big);
		if (consumed) {
			if (counter != big.sequence) {
				fprintf(stderr, "Sequence doesn't match count!"); 
				abort();
			}
			counter++;
		}
	}
}

typedef void (*QueueFunc)(void *);

void run_queue(void *queue, QueueFunc produce, QueueFunc consume, const char *name, int num_of_runs) {
	double avg = 0.0;

	double total_begin = get_current_time();

	for (int i = 0; i < num_of_runs; ++i) {
		double begin = get_current_time();
		Thread producer = create_thread(produce, queue);
		Thread consumer = create_thread(consume, queue);

		join_thread(producer);
		detach_thread(producer);

		join_thread(consumer);
		detach_thread(consumer);
		
		double end = get_current_time();
		avg += end - begin;
	}

	double total_end = get_current_time();

	double avg_total = avg / (double)num_of_runs;
	double total = total_end - total_begin;

	printf("[%s] Avg time per run: %g seconds\n", name,  avg_total);
	printf("[%s] Total time for all runs: %g seconds\n", name,  total);
}

int main(void) {
	printf("Message size: %zu\n", sizeof(BigStruct));

	int num_of_runs = 1000;

	////////////////////
	// Uncached Queue //
	////////////////////

	ParallelQueue pq = {0};
	parallel_queue_init(&pq, sizeof(BigStruct), QUEUE_CAPACITY);

	run_queue(&pq, produce_messages_pq, consume_messages_pq, "Uncached",  num_of_runs);

	//////////////////
	// Cached Queue //
	//////////////////

	CachedParallelQueue cq = {0};
	cached_parallel_queue_init(&cq, sizeof(BigStruct), QUEUE_CAPACITY);

	run_queue(&cq, produce_messages_cq, consume_messages_cq, "Cached",  num_of_runs);

	///////////////////////////
	// Cached Two-step Queue //
	///////////////////////////

	CachedParallelQueue cq_two_step = {0};
	cached_parallel_queue_init(&cq_two_step, sizeof(BigStruct), QUEUE_CAPACITY);

	run_queue(&cq_two_step, produce_two_step_messages_cq, consume_two_step_messages_cq, "Cached - Two-Step",  num_of_runs);

	Sleep(1000000);
}