#if defined(__APPLE__)
#import <mach/mach_time.h>
#endif  /* __APPLE__ */
#include <pthread.h>
#include <x86intrin.h>

typedef pthread_t threadid_t;
#define CURTHREAD_ID pthread_self
#define THREAD_EQUAL(x, y) pthread_equal(x, y)
#define ATOMIC_INC(x)  __sync_add_and_fetch(x, 1)
#define ATOMIC_DEC(x)  __sync_sub_and_fetch(x, 1)
#define ATOMIC_ADD(x, y)  __sync_add_and_fetch(x, y)

typedef struct pthread_lock {
	// An initialized flag is need because some futex's are 0-initialized
	// e.g. list instances created via PyType_GenericNew
	int initialized;
	pthread_mutex_t mutex;
} pthread_lock_t;
typedef pthread_lock_t primitivelock_t;

#define FUTEX_STATIC_INIT(description) { {1, PTHREAD_MUTEX_INITIALIZER}, description FUTEX_STATS_STATIC_INIT }

Py_LOCAL_INLINE(void) futex_init_primitive(primitivelock_t *lock) {
	pthread_lock_t* f = (pthread_lock_t*)lock;
	if (!f->initialized) {
		pthread_mutex_init(&f->mutex, NULL);
		f->initialized = 1;
	}
}

Py_LOCAL_INLINE(void) futex_lock_primitive(primitivelock_t *lock) {
	pthread_lock_t* f = (pthread_lock_t*)lock;
    /* assert(f->initialized); */
	if (!f->initialized) {
		futex_init_primitive(lock);
	}

	{
		int r = pthread_mutex_lock(&f->mutex);
		if (r != 0) {
			fprintf(stderr, "pthread_mutex_lock failed: %s\n", strerror(r));
		}
	}
}

Py_LOCAL_INLINE(void) futex_unlock_primitive(primitivelock_t *lock) {
	pthread_lock_t* f = (pthread_lock_t*)lock;
	int r = pthread_mutex_unlock(&f->mutex);
	if (r != 0) {
		fprintf(stderr, "pthread_mutex_unlock failed: %s\n", strerror(r));
	}
}
