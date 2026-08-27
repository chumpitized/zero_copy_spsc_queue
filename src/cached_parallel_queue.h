#ifndef CACHED_PARALLEL_QUEUE_H
#define CACHED_PARALLEL_QUEUE_H

#include "types.h"
#include "atomics.h"
#include <stdbool.h>

typedef struct ProducerState ProducerState;
struct ProducerState {
	usize 			cached_consumer_offset;
	atomic_usize 	offset;
};

typedef struct ConsumerState ConsumerState;
struct ConsumerState {
	usize 			cached_producer_offset;
	atomic_usize 	offset;
};

typedef struct CachedParallelQueue CachedParallelQueue;
struct CachedParallelQueue {
	unsigned char  *buffer;
	
	usize 			item_size;	            	
	usize 			capacity;
	usize			mask;

	ProducerState	producer __attribute__((aligned(64)));
	ConsumerState	consumer __attribute__((aligned(64)));
};

bool 		cached_parallel_queue_init				(CachedParallelQueue *q, usize item_size, usize capacity);

void   	   *cached_parallel_queue_produce_reserve	(CachedParallelQueue *q);
void 		cached_parallel_queue_produce_commit	(CachedParallelQueue *q);

bool 		cached_parallel_queue_produce			(CachedParallelQueue *q, const void *item);
usize 		cached_parallel_queue_produce_many		(CachedParallelQueue *q, usize count, const void *items);
												
const void *cached_parallel_queue_consume_reserve	(CachedParallelQueue *q);
void 		cached_parallel_queue_consume_commit	(CachedParallelQueue *q);

bool 		cached_parallel_queue_consume			(CachedParallelQueue *q, void *out);
usize 		cached_parallel_queue_consume_many		(CachedParallelQueue *q, usize count, void *out);
													
void 		cached_parallel_queue_destroy			(CachedParallelQueue *q);

#endif