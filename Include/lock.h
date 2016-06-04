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
/*
** futex
**
** linux-specific fast userspace lock
**
** Yes, this futex/furtex stuff shouldn't just be plopped in the
** middle of object.h.  We can find a better place for it later.
*/

#if defined(FUTEX_WANT_STATS) || defined(FURTEX_WANT_STATS)
#define PyMAX(a, b) ((a)>(b)?(a):(b))
#define PyMIN(a, b) ((a)<(b)?(a):(b))
#endif /* FUTEX_WANT_STATS || FURTEX_WANT_STATS */
#ifdef FUTEX_WANT_STATS
#define FUTEX_STATS_STATIC_INIT , 0, 0, 0, 0
#else
#define FUTEX_STATS_STATIC_INIT
#endif
#if defined(WIN32) || defined(WIN64)
#include "lock_win.h"
#elif defined(__APPLE__)
#include "lock_pthreads.h"
#else
#include "lock_linux.h"
#endif

#ifndef PY_TIME_FETCH_AND_ADD
#define PY_TIME_FETCH_AND_ADD(t, fieldname, delta) \
    if (t) { t->fieldname += delta; } else {        \
    __sync_fetch_and_add(&(py_time_refcounts.fieldname), delta); \
    }
#endif

#if 0 /* ACTIVATE STATS */
    /* Factor of two speed cost for PY_TIME_REFCOUNTS currently */
    #define PY_TIME_REFCOUNTS
    #define FUTEX_WANT_STATS
    #define FURTEX_WANT_STATS
    #define GC_TRACK_STATS
#endif /* ACTIVATE STATS */

typedef struct {
	uint64_t total_refcount_time;
	uint64_t total_refcounts;
} py_time_refcounts_t;

extern py_time_refcounts_t py_time_refcounts;

void py_time_refcounts_setzero(py_time_refcounts_t *t);

void py_time_refcounts_persist(py_time_refcounts_t *t);

#define CYCLES_PER_SEC 2600000000
#define F_CYCLES_PER_SEC ((double)CYCLES_PER_SEC)

Py_LOCAL_INLINE(uint64_t) fast_get_cycles(void) {
#ifdef __APPLE__
	return mach_absolute_time();
#else
	unsigned int t;
	return __rdtscp(&t);
#endif  /* __APPLE__ */
}

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

Py_LOCAL_INLINE(void) futex_init(futex_t *f) {
    futex_init_primitive(&f->futex);
}

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
Py_LOCAL_INLINE(float) seconds_from_cycles(uint64_t cycles) {
#ifdef __APPLE__
	static double to_nano = -1;
	mach_timebase_info_data_t sTimebaseInfo;
	if (to_nano < 0) {
		mach_timebase_info(&sTimebaseInfo);
		to_nano = (float)sTimebaseInfo.numer / (float)sTimebaseInfo.denom;
	}
	return cycles * to_nano / 1000000000;
#else
	return (float)(cycles / F_CYCLES_PER_SEC);
#endif
}


/*
** furtex
**
** recursive futex lock
**
*/
typedef struct {
	futex_t futex;
	int count;
	threadid_t tid;
	const char *description;
	const char *file;
	int line;
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
#define FURTEX_STATIC_INIT(description) { FUTEX_STATIC_INIT(description), 0, 0, description, NULL, 0, FURTEX_STATS_STATIC_INIT }
Py_LOCAL_INLINE(void) furtex_init(furtex_t *f) {
	memset(f, 0, sizeof(*f));
    futex_init(&f->futex);
}
#define furtex_lock(f) (_furtex_lock(f, __FILE__, __LINE__))
Py_LOCAL_INLINE(void) _furtex_lock(furtex_t *f, const char *file, int line) {
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
	f->file = file;
	f->line = line;
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

void furtex_stats(furtex_t *f);
void futex_stats(futex_t *f);

