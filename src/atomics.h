#ifndef ATOMICS_H
#define ATOMICS_H

#include <stdint.h>

typedef uint64_t atomic_usize;

static inline void atomic_store_relaxed_usize(atomic_usize *p, atomic_usize value) {
    __atomic_store_n(p, value, __ATOMIC_RELAXED);
}

static inline atomic_usize atomic_load_relaxed_usize(const atomic_usize *p) {
    return __atomic_load_n(p, __ATOMIC_RELAXED);
}

static inline void atomic_store_release_usize(atomic_usize *p, atomic_usize value) {
    __atomic_store_n(p, value, __ATOMIC_RELEASE);
}

static inline atomic_usize atomic_load_acquire_usize(const atomic_usize *p) {
    return __atomic_load_n(p, __ATOMIC_ACQUIRE);
}

#endif