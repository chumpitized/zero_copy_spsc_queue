#include <windows.h>
#include <assert.h>

#include "cached_parallel_queue.h"

static bool is_power_of_two(usize x) {
	return x != 0 && (x & (x - 1)) == 0;
}

bool cached_parallel_queue_init(CachedParallelQueue *q, usize item_size, usize capacity) {
	if (item_size == 0) 					return false;
	if (!is_power_of_two(capacity)) 		return false;
	if (capacity > SIZE_MAX / item_size)	return false;

	q->buffer = VirtualAlloc(NULL, capacity * item_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!q->buffer) return false;

	q->item_size 		= item_size;
	q->capacity 		= capacity;
	q->mask				= capacity - 1;

	ProducerState producer;
	producer.cached_consumer_offset = 0;
	producer.offset 				= 0;
	q->producer 					= producer;

	ConsumerState consumer;
	consumer.cached_producer_offset = 0;
	consumer.offset 				= 0;
	q->consumer 					= consumer;

	return true;
}

void *cached_parallel_queue_produce_reserve(CachedParallelQueue *q) {
	atomic_usize producer_offset = atomic_load_relaxed_usize(&q->producer.offset);

	if (unlikely(producer_offset - q->producer.cached_consumer_offset == q->capacity)) {
		q->producer.cached_consumer_offset = atomic_load_acquire_usize(&q->consumer.offset);

		if (producer_offset - q->producer.cached_consumer_offset == q->capacity) {
			return NULL;
		}
	}

	usize offset = (producer_offset & q->mask) * q->item_size;

	return &q->buffer[offset];
}

void cached_parallel_queue_produce_commit(CachedParallelQueue *q) {
	atomic_usize producer_offset = atomic_load_relaxed_usize(&q->producer.offset);
	atomic_store_release_usize(&q->producer.offset, producer_offset + 1);
}

bool cached_parallel_queue_produce(CachedParallelQueue *q, const void *item) {
	assert(item);

	atomic_usize producer_offset = atomic_load_relaxed_usize(&q->producer.offset);

	if (unlikely(producer_offset - q->producer.cached_consumer_offset == q->capacity)) {
		q->producer.cached_consumer_offset = atomic_load_acquire_usize(&q->consumer.offset);

		if (producer_offset - q->producer.cached_consumer_offset == q->capacity) {
			return false;
		}
	}

	usize offset = (producer_offset & q->mask) * q->item_size;

	memcpy(&q->buffer[offset], item, q->item_size);
	atomic_store_release_usize(&q->producer.offset, producer_offset + 1);

	return true;
}

const void *cached_parallel_queue_consume_reserve(CachedParallelQueue *q) {
	atomic_usize consumer_offset = atomic_load_relaxed_usize(&q->consumer.offset);	

	if (unlikely(consumer_offset == q->consumer.cached_producer_offset)) {
		q->consumer.cached_producer_offset = atomic_load_acquire_usize(&q->producer.offset);

		if (consumer_offset == q->consumer.cached_producer_offset) {
			return NULL;
		}
	}

	usize offset = (consumer_offset & q->mask) * q->item_size;

	return &q->buffer[offset];
}

void cached_parallel_queue_consume_commit(CachedParallelQueue *q) {
	atomic_usize consumer_offset = atomic_load_relaxed_usize(&q->consumer.offset);
	atomic_store_release_usize(&q->consumer.offset, consumer_offset + 1);
}

bool cached_parallel_queue_consume(CachedParallelQueue *q, void *out) {
	atomic_usize consumer_offset = atomic_load_relaxed_usize(&q->consumer.offset);

	if (unlikely(consumer_offset == q->consumer.cached_producer_offset)) {
		q->consumer.cached_producer_offset = atomic_load_acquire_usize(&q->producer.offset);

		if (consumer_offset == q->consumer.cached_producer_offset) {
			return false;
		}
	}

	usize offset = (consumer_offset & q->mask) * q->item_size;

	memcpy(out, &q->buffer[offset], q->item_size);
	atomic_store_release_usize(&q->consumer.offset, consumer_offset + 1);

	return true;
}

void cached_parallel_queue_destroy(CachedParallelQueue *q) {
	assert(q);
	assert(q->buffer);	

	bool ok = VirtualFree(q->buffer, 0, MEM_RELEASE);
	assert(ok);

	q->buffer 		= NULL;
	q->item_size 	= 0;
	q->capacity 	= 0;
	q->mask			= 0;
}