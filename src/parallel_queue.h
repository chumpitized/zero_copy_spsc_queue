#ifndef PARALLEL_QUEUE_H
#define PARALLEL_QUEUE_H

#include "types.h"
#include "atomics.h"
#include <stdbool.h>

typedef struct ParallelQueue ParallelQueue;
struct ParallelQueue {
	unsigned char  *buffer;
	
	usize 			item_size;	            	
	usize 			capacity;
	usize			mask;

	atomic_usize	producer_offset __attribute__((aligned(64)));
	atomic_usize	consumer_offset __attribute__((aligned(64)));
};

bool 	parallel_queue_init			(ParallelQueue *q, usize item_size, usize capacity);

bool 	parallel_queue_produce		(ParallelQueue *q, const void *item);
usize 	parallel_queue_produce_many	(ParallelQueue *q, usize count, const void *items);

bool 	parallel_queue_consume		(ParallelQueue *q, void *out);
usize 	parallel_queue_consume_many	(ParallelQueue *q, usize count, void *out);

void 	parallel_queue_destroy		(ParallelQueue *q);

#endif