#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

#define PY_TIME_FETCH_AND_ADD(t, fieldname, delta) \
    if (t) { t->fieldname += delta; } else {        \
    __sync_fetch_and_add(&(py_time_refcounts.fieldname), delta); \
    }

void py_time_refcounts_setzero(py_time_refcounts_t *t) {
#ifdef PY_TIME_REFCOUNTS
    t->total_refcount_time = 0;
    t->total_refcounts = 0;
#endif /* PY_TIME_REFCOUNTS */
}

py_time_refcounts_t py_time_refcounts;

py_time_refcounts_t* PyState_GetThisThreadPyTimeRefcounts(void);

void py_time_refcounts_persist(py_time_refcounts_t *t) {
#ifdef PY_TIME_REFCOUNTS
    py_time_refcounts_t* const zero = 0;
    PY_TIME_FETCH_AND_ADD(zero, total_refcount_time, t->total_refcount_time);
    PY_TIME_FETCH_AND_ADD(zero, total_refcounts, t->total_refcounts);
    py_time_refcounts_setzero(t);
#endif /* PY_TIME_REFCOUNTS */
}

void py_time_refcounts_stats(void) {
#ifdef PY_TIME_REFCOUNTS
    if (py_time_refcounts.total_refcounts) {
        printf("[py_incr/py_decr] %lu total calls\n", py_time_refcounts.total_refcounts);
        printf("[py_incr/py_decr] %lu total time spent, in cycles\n", py_time_refcounts.total_refcount_time);
        printf("[py_incr/py_decr] %f total time spent, in seconds\n", py_time_refcounts.total_refcount_time / 2600000000.0);
        printf("[py_incr/py_decr] %f average cycles for a py_incr/py_decr\n", ((double)py_time_refcounts.total_refcount_time) / py_time_refcounts.total_refcounts);
    }
#endif /* PY_TIME_REFCOUNTS */
}

#ifdef PY_TIME_REFCOUNTS

int __py_incref__(PyObject *o) {
    uint64_t start, delta;
    py_time_refcounts_t *t = PyState_GetThisThreadPyTimeRefcounts();
    _Py_INC_REFTOTAL;
    start = fast_get_cycles();
    __sync_fetch_and_add(&o->ob_refcnt.shared_refcnt, 1);
    delta = fast_get_cycles() - start;
    PY_TIME_FETCH_AND_ADD(t, total_refcount_time, delta);
    PY_TIME_FETCH_AND_ADD(t, total_refcounts, 1);
    return 1;
}

void __py_decref__(PyObject *o) {
    uint64_t start, delta, new_rc;
    py_time_refcounts_t *t = PyState_GetThisThreadPyTimeRefcounts();
    _Py_DEC_REFTOTAL;
    start = fast_get_cycles();
    new_rc = __sync_sub_and_fetch(&(o->ob_refcnt.shared_refcnt), 1);
    delta = fast_get_cycles() - start;
    PY_TIME_FETCH_AND_ADD(t, total_refcount_time, delta);
    PY_TIME_FETCH_AND_ADD(t, total_refcounts, 1);
    if (new_rc != 0)
        _Py_CHECK_REFCNT(o)
    else
        _Py_Dealloc(o);
}

#endif /* PY_TIME_REFCOUNTS */

void furtex_stats(furtex_t *f) {
#ifdef FURTEX_WANT_STATS
    printf("[%s] %ld total locks\n", f->description, (long)(f->no_contention_count + f->contention_count));
    printf("[%s] %ld locks without contention\n", f->description, (long)f->no_contention_count);
    printf("[%s] %ld locks with contention\n", f->description, (long)f->contention_count);
    if (f->contention_count) {
        printf("[%s] %ld contention total delay in cycles\n", f->description, (long)f->contention_total_delay);
        printf("[%s] %f contention total delay in cpu-seconds\n", f->description, f->contention_total_delay / 2600000000.0);
        printf("[%s] %f contention average delay in cycles\n", f->description, ((double)f->contention_total_delay) / f->contention_count);
        printf("[%s] %ld contention max delay in cycles\n", f->description, (long)f->contention_max_delta);
    }
    furtex_reset_stats(f);
#endif /* FURTEX_WANT_STATS */
}

void futex_stats(futex_t *f) {
#ifdef FUTEX_WANT_STATS
    printf("[%s] %ld total locks\n", f->description, (long)(f->no_contention_count + f->contention_count));
    printf("[%s] %ld locks without contention\n", f->description, (long)f->no_contention_count);
    printf("[%s] %ld locks with contention\n", f->description, (long)f->contention_count);
    if (f->contention_count) {
        printf("[%s] %ld contention total delay in cycles\n", f->description, (long)f->contention_total_delay);
        printf("[%s] %f contention total delay in cpu-seconds\n", f->description, f->contention_total_delay / F_CYCLES_PER_SEC);
        printf("[%s] %f contention average delay in cycles\n", f->description, ((double)f->contention_total_delay) / f->contention_count);
        printf("[%s] %ld contention max delay in cycles\n", f->description, (long)f->contention_max_delta);
    }
    futex_reset_stats(f);
/*
#else
    printf("[futex stats disabled at compile-time]\n");
*/
#endif /* FUTEX_WANT_STATS */
}

