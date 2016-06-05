#ifndef Py_LOCK_LINUX_H
#define Py_LOCK_LINUX_H

#include <linux/futex.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <x86intrin.h>


typedef pthread_t threadid_t;
#define CURRENT_THREAD_ID pthread_self()
#define ARE_THREADS_EQUAL(x, y) (pthread_equal((x), (y)))
#define ATOMIC_PRE_ADD_SSIZE_T(x, y)  __sync_add_and_fetch((x), (y))
#define ATOMIC_PRE_SUB_SSIZE_T(x, y)  __sync_sub_and_fetch((x), (y))


Py_LOCAL_INLINE(int) syscall_futex(int *futex, int operation, int value) {
    return syscall(SYS_futex, futex, operation, value, NULL, NULL, NULL);
}

#define futex_wait(futex, value) \
    (syscall_futex(futex, FUTEX_WAIT_PRIVATE, value))

#define futex_wake(futex, value) \
    (syscall_futex(futex, FUTEX_WAKE_PRIVATE, value))


#define PY_NATIVELOCK_T int

#define F_UNLOCKED           (0)
#define F_LOCKED             (1)
#define F_LOCKED_CONTENTION  (2)



#define PY_NATIVELOCK_STATIC_INIT() F_UNLOCKED

Py_LOCAL_INLINE(void) py_nativelock_init(PY_NATIVELOCK_T *nativelock) {
	*nativelock = F_UNLOCKED;
}


Py_LOCAL_INLINE(void) py_nativelock_lock(PY_NATIVELOCK_T *nativelock) {
	int current = __sync_val_compare_and_swap(nativelock, F_UNLOCKED, F_LOCKED);
	if (current == F_UNLOCKED)
		return;
	if (current != F_LOCKED_CONTENTION)
		current = __sync_lock_test_and_set(nativelock, F_LOCKED_CONTENTION);
	while (current != F_UNLOCKED) {
		futex_wait(nativelock, F_LOCKED_CONTENTION);
		current = __sync_lock_test_and_set(nativelock, F_LOCKED_CONTENTION);
	}
}

Py_LOCAL_INLINE(void) py_nativelock_unlock(PY_NATIVELOCK_T *nativelock) {
	if (__sync_fetch_and_sub(nativelock, 1) != F_LOCKED) {
		*nativelock = F_UNLOCKED;
		futex_wake(nativelock, 1);
	}
}

Py_LOCAL_INLINE(uint64_t) cycle_count_get(void) {
	unsigned int ignored;
	return __rdtscp(&ignored);
}

#define CYCLES_PER_SEC 2600000000

Py_LOCAL_INLINE(double) cycle_count_to_seconds(uint64_t cycles) {
    return cycles / (double)CYCLES_PER_SEC;
}

#endif /* Py_LOCK_LINUX_H */
