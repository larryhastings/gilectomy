#include "lock_win.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void futex_init_primitive(primitivelock_t *lock) {
	InitializeSRWLock((PSRWLOCK)lock);
}

void futex_lock_primitive(primitivelock_t *lock) {
	AcquireSRWLockExclusive((PSRWLOCK)lock);
}

void futex_unlock_primitive(primitivelock_t *lock) {
	ReleaseSRWLockExclusive((PSRWLOCK)lock);
}

unsigned long win32_threadid() {
	return GetCurrentThreadId();
}

Py_ssize_t Py_AtomicInc(Py_ssize_t* value) {
#if WIN64
	return InterlockedIncrement64(value);
#else
	return InterlockedIncrement(value);
#endif
}

Py_ssize_t Py_AtomicDec(Py_ssize_t* value) {
#if WIN64
	return InterlockedDecrement64(value);
#else
	return InterlockedDecrement(value);
#endif
}

Py_ssize_t Py_AtomicAdd(Py_ssize_t* to, Py_ssize_t value) {
#if WIN64
	return InterlockedAdd64(to, value);
#else
	return InterlockedAdd(to, value);
#endif
}

uint64_t _Py_AtomicAdd64(uint64_t* to, uint64_t value) {
	return InterlockedAdd64(to, value);
}
