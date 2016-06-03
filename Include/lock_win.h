#include <Python.h>

typedef void* primitivelock_t;
typedef unsigned long threadid_t;
#define CURTHREAD_ID win32_threadid
#define THREAD_EQUAL(x, y) (x == y)

#define ATOMIC_INC Py_AtomicInc
#define ATOMIC_DEC Py_AtomicDec
#define ATOMIC_ADD Py_AtomicAdd

#define FUTEX_STATIC_INIT(description) { {0}, description FUTEX_STATS_STATIC_INIT }

void futex_init_primitive(primitivelock_t *lock);
void futex_lock_primitive(primitivelock_t *lock);
void futex_unlock_primitive(primitivelock_t *lock);
unsigned long win32_threadid();

PyAPI_FUNC(long) Py_AtomicInc(long* value);
PyAPI_FUNC(long) Py_AtomicDec(long* value);
PyAPI_FUNC(long) Py_AtomicAdd(long* to, long value);
