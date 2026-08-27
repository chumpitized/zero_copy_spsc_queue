#include <windows.h>
#include <assert.h>

#include "parallel_queue.h"

static bool is_power_of_two(usize x) {
	return x != 0 && (x & (x - 1)) == 0;
}

bool parallel_queue_init(ParallelQueue *q, usize item_size, usize capacity) {
	if (item_size == 0) 					return false;
	if (!is_power_of_two(capacity)) 		return false;
	if (capacity > SIZE_MAX / item_size)	return false;

	q->buffer = VirtualAlloc(NULL, capacity * item_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!q->buffer) return false;

	q->item_size 		= item_size;
	q->capacity 		= capacity;
	q->mask				= capacity - 1;

	q->producer_offset 	= 0;
	q->consumer_offset 	= 0;

	return true;
}

bool parallel_queue_produce(ParallelQueue *q, const void *item) {
	assert(item);

	atomic_usize producer_offset = atomic_load_relaxed_usize(&q->producer_offset);
	atomic_usize consumer_offset = atomic_load_acquire_usize(&q->consumer_offset);

	if (unlikely(producer_offset - consumer_offset == q->capacity)) {
		return false;
	}

	usize offset = (producer_offset & q->mask) * q->item_size;

	memcpy(&q->buffer[offset], item, q->item_size);
	atomic_store_release_usize(&q->producer_offset, producer_offset + 1);

	return true;
}

usize parallel_queue_produce_many(ParallelQueue *q, usize count, const void *items) {
	atomic_usize producer_offset = atomic_load_relaxed_usize(&q->producer_offset);
	atomic_usize consumer_offset = atomic_load_acquire_usize(&q->consumer_offset);

	usize size 			= producer_offset - consumer_offset;
	usize free_slots 	= q->capacity - size;

	if (count > free_slots) {
		count = free_slots;
	}

	if (count == 0) {
		return 0;
	}

	usize non_wrap_index 			= (producer_offset & q->mask);
	usize non_wrap_byte_offset		= non_wrap_index * q->item_size;
	usize non_wrap_count 			= q->capacity - non_wrap_index;
	usize wrap_count				= count > non_wrap_count ? count - non_wrap_count : 0; 

	if (wrap_count) {
		memcpy(&q->buffer[non_wrap_byte_offset], items, q->item_size * non_wrap_count);
		memcpy(&q->buffer[0], (const u8 *)items + q->item_size * non_wrap_count, q->item_size * wrap_count);
	} else {
		memcpy(&q->buffer[non_wrap_byte_offset], items, q->item_size * count);
	}

	atomic_store_release_usize(&q->producer_offset, producer_offset + count);

	return count;
}

bool parallel_queue_consume(ParallelQueue *q, void *out) {
	atomic_usize consumer_offset = atomic_load_relaxed_usize(&q->consumer_offset);
	atomic_usize producer_offset = atomic_load_acquire_usize(&q->producer_offset);

	if (unlikely(consumer_offset == producer_offset)) {
		return false;
	}

	usize offset = (consumer_offset & q->mask) * q->item_size;

	memcpy(out, &q->buffer[offset], q->item_size);
	atomic_store_release_usize(&q->consumer_offset, consumer_offset + 1);

	return true;
}

usize parallel_queue_consume_many(ParallelQueue *q, usize count, void *out) {
	if (count == 0) {
		return 0;
	}

	atomic_usize consumer_offset = atomic_load_relaxed_usize(&q->consumer_offset);
	atomic_usize producer_offset = atomic_load_acquire_usize(&q->producer_offset);
	usize size = producer_offset - consumer_offset;

	if (size == 0) {
		return 0;
	}

	if (size < count) {
		count = size;
	}

	usize non_wrap_index 			= (consumer_offset & q->mask);
	usize non_wrap_byte_offset 		= non_wrap_index * q->item_size;
	usize non_wrap_count 			= q->capacity - non_wrap_index;
	usize wrap_count				= count > non_wrap_count ? count - non_wrap_count : 0;

	if (wrap_count) {
		memcpy(out, &q->buffer[non_wrap_byte_offset], q->item_size * non_wrap_count);
		memcpy((u8 *)out + q->item_size * non_wrap_count, &q->buffer[0], q->item_size * wrap_count);
	} else {
		memcpy(out, &q->buffer[non_wrap_byte_offset], q->item_size * count);
	}

	atomic_store_release_usize(&q->consumer_offset, consumer_offset + count);

	return count;
}

void parallel_queue_destroy(ParallelQueue *q) {
	assert(q);
	assert(q->buffer);	

	bool ok = VirtualFree(q->buffer, 0, MEM_RELEASE);
	assert(ok);

	q->buffer 		= NULL;
	q->item_size 	= 0;
	q->capacity 	= 0;
	q->mask			= 0;
}