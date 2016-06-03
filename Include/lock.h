// First, bring in platform specific locking support.  Platforms need to define the following:
//	futex_t	 - the core lock type
//	threadid_t - the type used for tracking the current thread ID
//  primitivelock_t - the primtive lock type
//  CURTHREAD_ID - a macro used for getting the current thread ID
//  THREAD_EQUAL - a macro used for comparing two thread IDs
//  ATOMIC_INC(x) - atomically increments a value and returns the new value
//  ATOMIC_DEC(x)  - atomically decrements a value and returns the new value
//	ATOMIC_ADD(x, value) - atomically adds value to x, and returns the new value
//  void futex_init_primitive(primitivelock_t *f) - initializes a futex
//  void futex_lock_primitive(primitivelock_t *f) - locks a futex
//  void futex_unlock_primitive(primitivelock_t *f) - unlocks a futex
//  FUTEX_STATIC_INIT(description) - statically initalizes a futex with a description

#ifdef FUTEX_WANT_STATS
#define FUTEX_STATS_STATIC_INIT , 0, 0, 0, 0
#else
#define FUTEX_STATS_STATIC_INIT
#endif

#if defined(WIN32) || defined(WIN64)
#include "lock_win.h"
#else
#include "lock_linux.h"
#endif

typedef struct {
	primitivelock_t futex;
	const char *description;
#ifdef FUTEX_WANT_STATS
	uint64_t no_contention_count;
	uint64_t contention_count;
	uint64_t contention_total_delay;
	uint64_t contention_max_delta;
#endif
} futex_t;

Py_LOCAL_INLINE(void) futex_lock(futex_t *f) {
#ifdef FUTEX_WANT_STATS
	unsigned int _;
	uint64_t start = __rdtscp(&_);
	uint64_t delta;
#endif /* FUTEX_WANT_STATS */
	futex_lock_primitive(&f->futex);
#ifdef FUTEX_WANT_STATS
	delta = __rdtscp(&_) - start;
	if (delta <= 250)
		f->no_contention_count++;
	else {
		f->contention_count++;
		f->contention_total_delay += delta;
		f->contention_max_delta = PyMAX(f->contention_max_delta, delta);
	}
#endif /* FUTEX_WANT_STATS */
	return;
}

Py_LOCAL_INLINE(void) futex_unlock(futex_t *f) {
	futex_unlock_primitive(&f->futex);
}

// Next the common locking functionality builds on top of those core locking primitives

Py_LOCAL_INLINE(void) futex_reset_stats(futex_t *f) {
#ifdef FUTEX_WANT_STATS
	f->no_contention_count =
		f->contention_total_delay =
		f->contention_max_delta =
		f->contention_count = 0;
#endif /* FUTEX_WANT_STATS */
}

Py_LOCAL_INLINE(void) futex_stats(futex_t *f) {
#ifdef FUTEX_WANT_STATS
	printf("[%s] %ld total locks\n", f->description, f->no_contention_count + f->contention_count);
	printf("[%s] %ld locks without contention\n", f->description, f->no_contention_count);
	printf("[%s] %ld locks with contention\n", f->description, f->contention_count);
	if (f->contention_count) {
		printf("[%s] %ld contention total delay in cycles\n", f->description, f->contention_total_delay);
		printf("[%s] %f contention total delay in cpu-seconds\n", f->description, f->contention_total_delay / F_CYCLES_PER_SEC);
		printf("[%s] %f contention average delay in cycles\n", f->description, ((double)f->contention_total_delay) / f->contention_count);
		printf("[%s] %ld contention max delay in cycles\n", f->description, f->contention_max_delta);
	}
	futex_reset_stats(f);
	/*
	#else
	printf("[futex stats disabled at compile-time]\n");
	*/
#endif /* FUTEX_WANT_STATS */
}


/*
** furtex
**
** recursive futex lock
**
*/

typedef struct {
	futex_t futex;
	const char *description;
	int count;
	threadid_t tid;
#ifdef FURTEX_WANT_STATS
	uint64_t no_contention_count;
	uint64_t contention_count;
	uint64_t contention_total_delay;
	uint64_t contention_max_delta;
#define FURTEX_STATS_STATIC_INIT , 0, 0, 0, 0
#else
#define FURTEX_STATS_STATIC_INIT
#endif /* FURTEX_WANT_STATS */
} furtex_t;

#define FURTEX_STATIC_INIT(description) { FUTEX_STATIC_INIT(description), description, 0, 0 FURTEX_STATS_STATIC_INIT }

Py_LOCAL_INLINE(void) furtex_init(furtex_t *f) {
	memset(f, 0, sizeof(*f));
}

Py_LOCAL_INLINE(void) furtex_lock(furtex_t *f) {
	threadid_t tid = CURTHREAD_ID();
#ifdef FURTEX_WANT_STATS
	uint64_t start, delta;
	unsigned int _;
#endif /* FURTEX_WANT_STATS */
	if (f->count && THREAD_EQUAL(f->tid, tid)) {
		f->count++;
		assert(f->count > 1);
		return;
	}
#ifdef FURTEX_WANT_STATS
	start = __rdtscp(&_);
#endif /* FURTEX_WANT_STATS */
	futex_lock(&(f->futex));
#ifdef FURTEX_WANT_STATS
	delta = __rdtscp(&_) - start;
	if (delta <= 250)
		f->no_contention_count++;
	else {
		f->contention_count++;
		f->contention_total_delay += delta;
		f->contention_max_delta = PyMAX(f->contention_max_delta, delta);
	}
#endif /* FURTEX_WANT_STATS */
	f->tid = tid;
	assert(f->count == 0);
	f->count = 1;
}

Py_LOCAL_INLINE(void) furtex_unlock(furtex_t *f) {
	/* this function assumes we own the lock! */
	assert(f->count > 0);
	if (--f->count)
		return;
	futex_unlock(&(f->futex));
}

Py_LOCAL_INLINE(void) furtex_reset_stats(furtex_t *f) {
#ifdef FURTEX_WANT_STATS
	f->no_contention_count =
		f->contention_total_delay =
		f->contention_max_delta =
		f->contention_count = 0;
#endif /* FURTEX_WANT_STATS */
}

Py_LOCAL_INLINE(void) furtex_stats(furtex_t *f) {
#ifdef FURTEX_WANT_STATS
	printf("[%s] %ld total locks\n", f->description, f->no_contention_count + f->contention_count);
	printf("[%s] %ld locks without contention\n", f->description, f->no_contention_count);
	printf("[%s] %ld locks with contention\n", f->description, f->contention_count);
	if (f->contention_count) {
		printf("[%s] %ld contention total delay in cycles\n", f->description, f->contention_total_delay);
		printf("[%s] %f contention total delay in cpu-seconds\n", f->description, f->contention_total_delay / 2600000000.0);
		printf("[%s] %f contention average delay in cycles\n", f->description, ((double)f->contention_total_delay) / f->contention_count);
		printf("[%s] %ld contention max delay in cycles\n", f->description, f->contention_max_delta);
	}
	furtex_reset_stats(f);
	/*
	#else
	printf("[furtex stats disabled at compile-time]\n");
	*/
#endif /* FURTEX_WANT_STATS */
}

