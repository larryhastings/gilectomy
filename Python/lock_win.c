#include "lock_win.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#oid py_nativelock_init(py_nativelock_t *nativelock) {
	InitializeSRWLock((PSRWLOCK)nativelock);
}

void py_nativelock_lock(py_nativelock_t *nativelock) {
	AcquireSRWLockExclusive((PSRWLOCK)nativelock);
}

void py_nativelock_unlock(cheaplock_t *nativelock) {
	ReleaseSRWLockExclusive((PSRWLOCK)nativelock);
}

unsigned long win32_threadid() {
	return GetCurrentThreadId();
}

Py_ssize_t Py_AtomicAddSSize_t(Py_ssize_t *to, Py_ssize_t value) {
#if WIN64
	return (Py_ssize_t)InterlockedAdd64((LONGLONG *)to, (LONGLONG)value);
#else
	return (Py_ssize_t)InterlockedAdd((LONG *)to, (LONG)value);
#endif
}

Py_ssize_t Py_AtomicSubSSize_t(Py_ssize_t *to, Py_ssize_t value) {
#if WIN64
	return (Py_ssize_t)InterlockedAdd64((LONGLONG *)to, -(LONGLONG)value);
#else
	return (Py_ssize_t)InterlockedAdd((LONG *)to, -(LONG)value);
#endif
}
