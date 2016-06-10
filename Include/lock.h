#ifndef __PY_LOCK_H
#define __PY_LOCK_H

// First, bring in platform specific locking support.  Platforms need to define the following:
//
//  ATOMIC_PRE_ADD_SSIZE_T(x) - atomically increments an ssize_t value and returns the new value
//  ATOMIC_PRE_SUB_SSIZE_T(x) - atomically decrements an ssize_t value and returns the new value
//
//  cycle_count_get() - return some cheap high-precision clock (e.g. RDTSCP)
//  cycle_count_to_seconds() - convert cycle count to floating-point seconds
//  cycle counts should always be type uint64_t.
//
//	threadid_t - the handle type for unique thread identifiers
//  CURRENT_THREAD_ID - a macro used for getting the current thread ID
//  ARE_THREADS_EQUAL - a macro used for comparing two thread IDs
//
//  PY_NATIVELOCK_T - the cheapest native lock available
//             doesn't have to be recursive
//             doesn't need to support multiple readers
//             this should be a #define
//  PY_NATIVELOCK_STATIC_INIT() - statically initalizes a PY_NATIVELOCK_T
//  void py_nativelock_init(PY_NATIVELOCK_T *) - dynamically initializes a native lock
//  void py_nativelock_lock(PY_NATIVELOCK_T *) - locks a native lock
//  void py_nativelock_unlock(PY_NATIVELOCK_T *) - unlocks a native lock
//  (we always allocate the nativelock, we always pass a pointer to the nativelock to you)
//
// We then define for you:
//
//	py_lock_t	 - the actual basic lock we use
//             built on top of py_nativelock_t
//             adds statistics
//  PY_LOCK_STATIC_INIT(description) - statically initalizes a py_lock with a description
//
//
//  py_recursive_lock_t - a recursive lock probably built on top of py_lock_t

#if 0 /* ACTIVATE STATS */
	/* Turn on statistics tracking for locks. Expensive and slow! */
    #define PY_TIME_REFCOUNTS
    #define PY_LOCK_WANT_STATS
    #define PY_RECURSIVE_LOCK_WANT_STATS
    #define GC_TRACK_STATS
#endif /* ACTIVATE STATS */


Py_LOCAL_INLINE(uint64_t) cycle_count_get(void);
Py_LOCAL_INLINE(double)   cycle_count_to_seconds(uint64_t cycles);

#define ATOMIC_INC(refcnt) ATOMIC_PRE_ADD_SSIZE_T((refcnt), 1)
#define ATOMIC_DEC(refcnt) ATOMIC_PRE_SUB_SSIZE_T((refcnt), 1)

#if defined(PY_LOCK_WANT_STATS) || defined(PY_RECURSIVE_LOCK_WANT_STATS)
#define PyMAX(a, b) ((a)>(b)?(a):(b))
#define PyMIN(a, b) ((a)<(b)?(a):(b))
#endif /* PY_LOCK_WANT_STATS || PY_RECURSIVE_LOCK_WANT_STATS */

#ifdef PY_LOCK_WANT_STATS
#define PY_LOCK_STATS_STATIC_INIT , 0, 0, 0, 0
#else
#define PY_LOCK_STATS_STATIC_INIT
#endif

#if defined(WIN32) || defined(WIN64)
#include "lock_win.h"
#elif defined(__APPLE__)
#include "lock_pthreads.h"
#else
#include "lock_linux.h"
#endif

Py_LOCAL_INLINE(void) py_nativelock_init  (PY_NATIVELOCK_T *nativelock);
Py_LOCAL_INLINE(void) py_nativelock_lock  (PY_NATIVELOCK_T *nativelock);
Py_LOCAL_INLINE(void) py_nativelock_unlock(PY_NATIVELOCK_T *nativelock);


typedef struct {
	Py_ssize_t total_refcount_time;
	Py_ssize_t total_refcounts;
} py_time_refcounts_t;

extern py_time_refcounts_t py_time_refcounts;

void py_time_refcounts_setzero(py_time_refcounts_t *t);
void py_time_refcounts_persist(py_time_refcounts_t *t);

#ifndef PY_TIME_FETCH_AND_ADD
#define PY_TIME_FETCH_AND_ADD(t, fieldname, delta) \
	do { \
	    if (t) \
	    	t->fieldname += delta; \
	    else { \
	    	ATOMIC_PRE_ADD_SSIZE_T(&(py_time_refcounts.fieldname), delta); \
	    } \
    } while (0) \

#endif



typedef struct {
	PY_NATIVELOCK_T nativelock;
	const char *description;
	const char *file;
	int line;
#ifdef PY_LOCK_WANT_STATS
	uint64_t no_contention_count;
	uint64_t contention_count;
	uint64_t contention_total_delay;
	uint64_t contention_max_delta;
#endif
} py_lock_t;

Py_LOCAL_INLINE(void) py_lock_init(py_lock_t *lock, const char *description) {
	memset(lock, 0, sizeof(*lock));
	py_nativelock_init(&(lock->nativelock));
	lock->description = description;
}

#define PY_LOCK_STATIC_INIT(description) \
	{ PY_NATIVELOCK_STATIC_INIT(), description, NULL, 0 PY_LOCK_STATS_STATIC_INIT }


#define py_lock_lock(lock) (_py_lock_lock((lock), __FILE__, __LINE__))

Py_LOCAL_INLINE(void) _py_lock_lock(py_lock_t *lock, const char *file, int line) {

#ifdef PY_LOCK_WANT_STATS
	uint64_t start = cycle_count_get();
	uint64_t delta;
#endif /* PY_LOCK_WANT_STATS */

	py_nativelock_lock(&(lock->nativelock));
	lock->file = file;
	lock->line = line;

#ifdef PY_LOCK_WANT_STATS
	delta = cycle_count_get() - start;
	if (delta <= 250)
		lock->no_contention_count++;
	else {
		lock->contention_count++;
		lock->contention_total_delay += delta;
		lock->contention_max_delta = PyMAX(lock->contention_max_delta, delta);
	}
#endif /* PY_LOCK_WANT_STATS */
}

Py_LOCAL_INLINE(void) py_lock_unlock(py_lock_t *lock) {
	/* negative number means unlocked! */
	lock->line = -lock->line;
	py_nativelock_unlock(&(lock->nativelock));
}

Py_LOCAL_INLINE(void) py_lock_reset_stats(py_lock_t *lock) {
#ifdef PY_LOCK_WANT_STATS
	lock->no_contention_count =
		lock->contention_total_delay =
		lock->contention_max_delta =
		lock->contention_count = 0;
#else
	(void)lock;
#endif /* PY_LOCK_WANT_STATS */
}


/*
** py_recursive_lock_t
**
** recursive lock
**
*/
typedef struct {
	py_lock_t lock;
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
#define PY_RECURSIVELOCK_STATS_STATIC_INIT , 0, 0, 0, 0
#else
#define PY_RECURSIVELOCK_STATS_STATIC_INIT
#endif /* PY_RECURSIVELOCK_WANT_STATS */
} py_recursivelock_t;

#define PY_RECURSIVELOCK_STATIC_INIT(description) \
	{ PY_LOCK_STATIC_INIT(description), 0, 0, description, NULL, 0 PY_RECURSIVELOCK_STATS_STATIC_INIT }

Py_LOCAL_INLINE(void) py_recursivelock_init(py_recursivelock_t *f, const char *description) {
	memset(f, 0, sizeof(*f));
	py_lock_init(&(f->lock), description);
}


#define py_recursivelock_lock(recursivelock) \
	(_py_recursivelock_lock(recursivelock, __FILE__, __LINE__))

Py_LOCAL_INLINE(void) _py_recursivelock_lock(py_recursivelock_t *recursivelock, const char *file, int line) {
	threadid_t tid = CURRENT_THREAD_ID;

#ifdef FURTEX_WANT_STATS
	uint64_t start = cycle_count_get();
	uint64_t delta;
#endif /* PY_RECURSIVELOCK_WANT_STATS */

	if (recursivelock->count && ARE_THREADS_EQUAL(recursivelock->tid, tid)) {
		recursivelock->count++;
		assert(recursivelock->count > 1);
		return;
	}

	_py_lock_lock(&(recursivelock->lock), file, line);

#ifdef PY_RECURSIVELOCK_WANT_STATS
	delta = cycle_count_get() - start;
	if (delta <= 250)
		recursivelock->no_contention_count++;
	else {
		recursivelock->contention_count++;
		recursivelock->contention_total_delay += delta;
		recursivelock->contention_max_delta = PyMAX(recursivelock->contention_max_delta, delta);
	}
#endif /* PY_RECURSIVELOCK_WANT_STATS */

	recursivelock->file = file;
	recursivelock->line = line;
	recursivelock->tid = tid;
	assert(recursivelock->count == 0);
	recursivelock->count = 1;
}

Py_LOCAL_INLINE(void) py_recursivelock_unlock(py_recursivelock_t *recursivelock) {
	/* this function assumes we own the lock! */
	assert(recursivelock->count > 0);
	if (--recursivelock->count)
		return;
	/* negative number means unlocked! */
	recursivelock->line = -recursivelock->line;
	py_lock_unlock(&(recursivelock->lock));
}

Py_LOCAL_INLINE(void) py_recursivelock_reset_stats(py_recursivelock_t *recursivelock) {
#ifdef PY_RECURSIVELOCK_WANT_STATS
	recursivelock->no_contention_count =
		recursivelock->contention_total_delay =
		recursivelock->contention_max_delta =
		recursivelock->contention_count = 0;
#else
	(void)recursivelock;
#endif /* PY_RECURSIVELOCK_WANT_STATS */
}

Py_LOCAL_INLINE(int) py_recursivelock_owned_by_me(py_recursivelock_t *recursivelock) {
    return (recursivelock->count && ARE_THREADS_EQUAL(recursivelock->tid, tid));
}

void py_lock_stats(py_lock_t *lock);
void py_recursivelock_stats(py_recursivelock_t *recursivelock);


#endif /* __PY_LOCK_H */
