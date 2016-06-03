#ifndef Py_LOCK_LINUX_H
#define Py_LOCK_LINUX_H

#include <linux/futex.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <x86intrin.h>

typedef pthread_t threadid_t;
typedef int primitivelock_t;
#define CURTHREAD_ID pthread_self
#define THREAD_EQUAL(x, y) pthread_equal(x, y)
#define ATOMIC_INC(x)  __sync_add_and_fetch(x, 1)
#define ATOMIC_DEC(x)  __sync_sub_and_fetch(x, 1)
#define ATOMIC_ADD(x, y)  __sync_add_and_fetch(x, y)

Py_LOCAL_INLINE(int) syscall_futex(int *futex, int operation, int value) {
    return syscall(SYS_futex, futex, operation, value, NULL, NULL, NULL);
}

#define futex_wait(futex, value) \
    (syscall_futex(futex, FUTEX_WAIT_PRIVATE, value))

#define futex_wake(futex, value) \
    (syscall_futex(futex, FUTEX_WAKE_PRIVATE, value))



#define FUTEX_STATIC_INIT(description) { 0, description FUTEX_STATS_STATIC_INIT }

Py_LOCAL_INLINE(void) futex_init_primitive(int *lock) {
	*lock = 0;
}

Py_LOCAL_INLINE(void) futex_lock_primitive(int *lock) {
	int current = __sync_val_compare_and_swap(lock, 0, 1);
	if (current == 0)
		return;
	if (current != 2)
		current = __sync_lock_test_and_set(lock, 2);
	while (current != 0) {
		futex_wait(lock, 2);
		current = __sync_lock_test_and_set(lock, 2);
	}
}

Py_LOCAL_INLINE(void) futex_unlock_primitive(int *lock) {
	if (__sync_fetch_and_sub(lock, 1) != 1) {
		*lock = 0;
		futex_wake(lock, 1);
	}
}

PyAPI_FUNC(long) Py_AtomicInc(long* value);
PyAPI_FUNC(long) Py_AtomicDec(long* value);
PyAPI_FUNC(long) Py_AtomicAdd(long* to, long value);


#endif