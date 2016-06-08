#ifndef Py_LOCK_PTHREADS_H
#define Py_LOCK_PTHREADS_H

#if defined(__APPLE__)
#import <mach/mach_time.h>
#endif  /* __APPLE__ */
#include <pthread.h>
#include <x86intrin.h>


typedef pthread_t threadid_t;
#define CURRENT_THREAD_ID pthread_self()
#define ARE_THREADS_EQUAL(x, y) (pthread_equal((x), (y)))
#define ATOMIC_PRE_ADD_SSIZE_T(x, y)  __sync_add_and_fetch((x), (y))
#define ATOMIC_PRE_SUB_SSIZE_T(x, y)  __sync_sub_and_fetch((x), (y))


typedef struct py_nativelock_t {
	// An initialized flag is need because some futexes
	// are 0-initialized
	// e.g. list instances created via PyType_GenericNew
	int initialized;
	pthread_mutex_t mutex;
} py_nativelock_t;

#define PY_NATIVELOCK_T py_nativelock_t

#define PY_NATIVELOCK_STATIC_INIT() {1, PTHREAD_MUTEX_INITIALIZER}


Py_LOCAL_INLINE(void) py_nativelock_init(PY_NATIVELOCK_T *nativelock) {
	if (!nativelock->initialized) {
		pthread_mutex_init(&(nativelock->mutex), NULL);
		nativelock->initialized = 1;
	}
}


Py_LOCAL_INLINE(void) py_nativelock_lock(PY_NATIVELOCK_T *nativelock) {
	int r;

	if (!nativelock->initialized) {
		py_nativelock_init(nativelock);
	}

	r = pthread_mutex_lock(&(nativelock->mutex));
	if (r != 0) {
		fprintf(stderr, "py_nativelock_lock(): pthread_mutex_lock failed: %s\n", strerror(r));
	}
}


Py_LOCAL_INLINE(void) py_nativelock_unlock(PY_NATIVELOCK_T *nativelock) {
	int r = pthread_mutex_unlock(&(nativelock->mutex));
	if (r != 0) {
		fprintf(stderr, "py_nativelock_unlock(): pthread_mutex_unlock failed: %s\n", strerror(r));
	}
}


Py_LOCAL_INLINE(uint64_t) cycle_count_get(void) {
	return (uint64_t)mach_absolute_time();
}


Py_LOCAL_INLINE(double) cycle_count_to_seconds(uint64_t cycles) {
	static double cycles_to_nanoseconds = -1;

	if (cycles_to_nanoseconds == -1) {
		mach_timebase_info_data_t sTimebaseInfo;
		mach_timebase_info(&sTimebaseInfo);
		cycles_to_nanoseconds = (double)sTimebaseInfo.numer / (double)sTimebaseInfo.denom;
	}

	return (cycles * cycles_to_nanoseconds) / 1000000000;
}

#endif /* Py_LOCK_PTHREADS_H */
