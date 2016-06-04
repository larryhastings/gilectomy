#ifndef __APPLE__
#include "Python.h"
#include "lock_linux.h"

Py_ssize_t Py_AtomicInc(Py_ssize_t* value) {
	return ATOMIC_INC(value);
}

Py_ssize_t Py_AtomicDec(Py_ssize_t* value) {
	return ATOMIC_DEC(value);
}

Py_ssize_t Py_AtomicAdd(Py_ssize_t* to, Py_ssize_t value) {
	return ATOMIC_ADD(to, value);
}
#endif