#ifndef Py_LOCK_WIN_H
#define Py_LOCK_WIN_H

#include <Python.h>
#include <intrin.h>


typedef unsigned long threadid_t;
#define CURRENT_THREAD_ID win32_threadid
#define ARE_THREADS_EQUAL(x, y) (pthread_equal((x), (y)))
#define ATOMIC_PRE_ADD_SSIZE_T(x, y)  Py_AtomicAddSSize_t((x), (y))
#define ATOMIC_PRE_SUB_SSIZE_T(x, y)  Py_AtomicSubSSize_t((x), (y))

PyAPI_FUNC(Py_ssize_t) Py_AtomicAddSSize_t(Py_ssize_t *to, Py_ssize_t value);
PyAPI_FUNC(Py_ssize_t) Py_AtomicSubSSize_t(Py_ssize_t *to, Py_ssize_t value);

/* we just cast to SRWLOCK */
#define PY_NATIVELOCK_T void *

// SRWLOCK_INIT would be correct, but we don't want
// to include windows.h here for some reason
// #define PY_NATIVELOCK_STATIC_INIT() {SRWLOCK_INIT}
#define PY_NATIVELOCK_STATIC_INIT() {0}

unsigned long win32_threadid();

#endif /* Py_LOCK_WIN_H */
