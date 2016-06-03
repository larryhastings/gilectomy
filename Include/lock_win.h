#include <Python.h>
#include <intrin.h>

typedef void* primitivelock_t;
typedef unsigned long threadid_t;
#define CURTHREAD_ID win32_threadid
#define THREAD_EQUAL(x, y) (x == y)

#define ATOMIC_INC Py_AtomicInc
#define ATOMIC_DEC Py_AtomicDec
#define ATOMIC_ADD Py_AtomicAdd

uint64_t _Py_AtomicAdd64(uint64_t * to, uint64_t value);

#define PY_TIME_FETCH_AND_ADD(t, fieldname, delta) \
    if (t) { t->fieldname += delta; } else {        \
    _Py_AtomicAdd64(&(py_time_refcounts.fieldname), delta); \
    }

#define FUTEX_STATIC_INIT(description) { {0}, description FUTEX_STATS_STATIC_INIT }

void futex_init_primitive(primitivelock_t *lock);
void futex_lock_primitive(primitivelock_t *lock);
void futex_unlock_primitive(primitivelock_t *lock);
unsigned long win32_threadid();
