/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PERCPU_COUNTER_H
#define _LINUX_PERCPU_COUNTER_H
/*
 * A simple "approximate counter" for use in ext2 and ext3 superblocks.
 *
 * WARNING: these things are HUGE.  4 kbytes per counter on 32-way P4.
 */

#include "spinlock.h"
#include "stdbool.h"
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h> // For alignas



/* percpu_counter batch for local add or sub */
#define PERCPU_COUNTER_LOCAL_BATCH	INT32_MAX

#define NCPU 84

// Assuming a 64-byte cache line
#define CACHE_LINE_SIZE 64
#define cachepadded_int32_t alignas(CACHE_LINE_SIZE) int32_t


struct percpu_counter {
	spinlock_t lock;
	int64_t count;
	cachepadded_int32_t **counters;
};

extern int percpu_counter_batch;

int percpu_counter_init(struct percpu_counter *fbc, int64_t amount);

void percpu_counter_destroy(struct percpu_counter *fbc);

void percpu_counter_set(struct percpu_counter *fbc, int64_t amount);
void percpu_counter_add_batch(struct percpu_counter *fbc, int64_t amount,
			      int32_t batch, uint32_t cpu_id);
int64_t __percpu_counter_sum(struct percpu_counter *fbc);
int __percpu_counter_compare(struct percpu_counter *fbc, int64_t rhs, int32_t batch);
bool __percpu_counter_limited_add(struct percpu_counter *fbc, int64_t limit,
				  int64_t amount, int32_t batch);
void percpu_counter_sync(struct percpu_counter *fbc);

static inline int percpu_counter_compare(struct percpu_counter *fbc, int64_t rhs)
{
	return __percpu_counter_compare(fbc, rhs, percpu_counter_batch);
}

static inline void percpu_counter_add(struct percpu_counter *fbc, int64_t amount, uint32_t cpu_id )
{
	percpu_counter_add_batch(fbc, amount, percpu_counter_batch, cpu_id);
}

// static inline bool
// percpu_counter_limited_add(struct percpu_counter *fbc, int64_t limit, int64_t amount)
// {
// 	return __percpu_counter_limited_add(fbc, limit, amount,
// 					    percpu_counter_batch);
// }

/*
 * With percpu_counter_add_local() and percpu_counter_sub_local(), counts
 * are accumulated in local per cpu counter and not in fbc->count until
 * local count overflows PERCPU_COUNTER_LOCAL_BATCH. This makes counter
 * write efficient.
 * But percpu_counter_sum(), instead of percpu_counter_read(), needs to be
 * used to add up the counts from each CPU to account for all the local
 * counts. So percpu_counter_add_local() and percpu_counter_sub_local()
 * should be used when a counter is updated frequently and read rarely.
 */
static inline void
percpu_counter_add_local(struct percpu_counter *fbc, int64_t amount, uint32_t cpu_id)
{
	percpu_counter_add_batch(fbc, amount, PERCPU_COUNTER_LOCAL_BATCH, cpu_id);
}

static inline int64_t percpu_counter_sum_positive(struct percpu_counter *fbc)
{
	int64_t ret = __percpu_counter_sum(fbc);
	return ret < 0 ? 0 : ret;
}

static inline int64_t percpu_counter_sum(struct percpu_counter *fbc)
{
	return __percpu_counter_sum(fbc);
}

static inline int64_t percpu_counter_read(struct percpu_counter *fbc)
{
	return fbc->count;
}

/*
 * It is possible for the percpu_counter_read() to return a small negative
 * number for some counter which should never be negative.
 *
 */
static inline int64_t percpu_counter_read_positive(struct percpu_counter *fbc)
{
	/* Prevent reloads of fbc->count */
	int64_t ret = READ_ONCE(fbc->count);

	if (ret >= 0)
		return ret;
	return 0;
}

static inline bool percpu_counter_initialized(struct percpu_counter *fbc)
{
	return (fbc->counters != NULL);
}
#endif