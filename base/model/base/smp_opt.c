#include "smp_opt.h"
#include "base/def.h"
#include "dbg/chk.h"

#define SMP_OPT_SPIN_LIMIT 1024u

void smp_opt_lock_init(smp_opt_lock_t *lock)
{
    DBG_CHECK(lock != NULL);
    lock->val = 0;
}

void smp_opt_lock_acquire(smp_opt_lock_t *lock)
{
    DBG_CHECK(lock != NULL);
    while (__atomic_exchange_n(&lock->val, 1u, __ATOMIC_ACQUIRE)) {
        cpu_relax();
    }
}

void smp_opt_lock_release(smp_opt_lock_t *lock)
{
    DBG_CHECK(lock != NULL);
    __atomic_store_n(&lock->val, 0u, __ATOMIC_RELEASE);
}

void smp_opt_snapshot_seq_init(smp_opt_snapshot_seq_t *seq)
{
    DBG_CHECK(seq != NULL);
    seq->val = 0;
}

u32 smp_opt_snapshot_seq_write_begin(smp_opt_snapshot_seq_t *seq)
{
    DBG_CHECK(seq != NULL);
    u32 seq0 = __atomic_load_n(&seq->val, __ATOMIC_RELAXED);
    __atomic_store_n(&seq->val, seq0 + 1u, __ATOMIC_RELEASE);
    return seq0;
}

void smp_opt_snapshot_seq_write_end(smp_opt_snapshot_seq_t *seq, u32 seq0)
{
    DBG_CHECK(seq != NULL);
    __atomic_store_n(&seq->val, seq0 + 2u, __ATOMIC_RELEASE);
}

u32 smp_opt_snapshot_seq_read_begin(smp_opt_snapshot_seq_t *seq)
{
    DBG_CHECK(seq != NULL);
    for (;;) {
        u32 seq0 = __atomic_load_n(&seq->val, __ATOMIC_ACQUIRE);
        if ((seq0 & 1u) == 0u) {
            return seq0;
        }
        cpu_relax();
    }
}

bool smp_opt_snapshot_seq_read_retry(smp_opt_snapshot_seq_t *seq, u32 seq0)
{
    DBG_CHECK(seq != NULL);
    u32 seq1 = __atomic_load_n(&seq->val, __ATOMIC_ACQUIRE);
    return seq0 != seq1;
}

#if defined(__linux__)

#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

struct smp_opt {
    pthread_t thread;
    const u64 *host_cycle;
    u64 cycle_shadow;
    smp_opt_task_func_t task_func;
    void *task_args;
    u32 run_epoch;
    u32 done_epoch;
    u32 run_sleepers;
    u32 done_sleepers;
    u32 stop;
};

static inline u32 smp_atomic_load_u32(const u32 *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
}

static inline void smp_atomic_store_u32(u32 *ptr, u32 val)
{
    __atomic_store_n(ptr, val, __ATOMIC_RELEASE);
}

static inline u32 smp_atomic_inc_u32(u32 *ptr)
{
    return __atomic_add_fetch(ptr, 1u, __ATOMIC_RELEASE);
}

static inline void smp_atomic_dec_u32(u32 *ptr)
{
    (void)__atomic_sub_fetch(ptr, 1u, __ATOMIC_RELEASE);
}

static void smp_futex_wait(u32 *addr, u32 expected)
{
    int ret = syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, NULL, NULL, 0);
    DBG_CHECK(ret == 0 || errno == EAGAIN || errno == EINTR);
}

static void smp_futex_wake_all(u32 *addr)
{
    int ret = syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0);
    DBG_CHECK(ret >= 0);
}

static bool smp_wait_epoch_change(smp_opt_t *smp, u32 seen_epoch, u32 *next_epoch)
{
    for (u32 spin = 0; spin < SMP_OPT_SPIN_LIMIT; spin++) {
        if (smp_atomic_load_u32(&smp->stop)) {
            return false;
        }
        u32 epoch = smp_atomic_load_u32(&smp->run_epoch);
        if (epoch != seen_epoch) {
            *next_epoch = epoch;
            return true;
        }
        cpu_relax();
    }

    if (!smp_atomic_load_u32(&smp->stop) &&
        smp_atomic_load_u32(&smp->run_epoch) == seen_epoch) {
        smp_atomic_inc_u32(&smp->run_sleepers);
        while (!smp_atomic_load_u32(&smp->stop) &&
               smp_atomic_load_u32(&smp->run_epoch) == seen_epoch) {
            smp_futex_wait(&smp->run_epoch, seen_epoch);
        }
        smp_atomic_dec_u32(&smp->run_sleepers);
    }

    if (smp_atomic_load_u32(&smp->stop)) {
        return false;
    }

    *next_epoch = smp_atomic_load_u32(&smp->run_epoch);
    return true;
}

static void *smp_task_thread_main(void *args)
{
    smp_opt_t *smp = args;
    u32 seen_epoch = smp_atomic_load_u32(&smp->run_epoch);

    for (;;) {
        u32 epoch;
        if (!smp_wait_epoch_change(smp, seen_epoch, &epoch)) {
            return NULL;
        }

        DBG_CHECK(smp->task_func != NULL);
        smp->task_func(smp->task_args);
        smp_atomic_store_u32(&smp->done_epoch, epoch);
        if (smp_atomic_load_u32(&smp->done_sleepers) != 0u) {
            smp_futex_wake_all(&smp->done_epoch);
        }
        seen_epoch = epoch;
    }
}

bool smp_opt_supported(void)
{
    return true;
}

const char *smp_opt_unsupported_reason(void)
{
    return "";
}

smp_opt_t *smp_opt_create(const u64 *host_cycle,
    smp_opt_task_func_t task_func, void *task_args)
{
    DBG_CHECK(host_cycle != NULL);
    DBG_CHECK(task_func != NULL);

    smp_opt_t *smp = calloc(1, sizeof(*smp));
    DBG_CHECK(smp != NULL);
    smp->host_cycle = host_cycle;
    smp->cycle_shadow = *host_cycle;
    smp->task_func = task_func;
    smp->task_args = task_args;

    int ret = pthread_create(&smp->thread, NULL, &smp_task_thread_main, smp);
    DBG_CHECK(ret == 0);
    return smp;
}

void smp_opt_destroy(smp_opt_t *smp)
{
    if (smp == NULL) {
        return;
    }

    smp_atomic_store_u32(&smp->stop, 1u);
    smp_atomic_inc_u32(&smp->run_epoch);
    if (smp_atomic_load_u32(&smp->run_sleepers) != 0u) {
        smp_futex_wake_all(&smp->run_epoch);
    }
    int ret = pthread_join(smp->thread, NULL);
    DBG_CHECK(ret == 0);
    free(smp);
}

void smp_opt_submit(smp_opt_t *smp)
{
    DBG_CHECK(smp != NULL);
    smp_opt_drain(smp);
    smp->cycle_shadow = *smp->host_cycle;
    smp_atomic_inc_u32(&smp->run_epoch);
    if (smp_atomic_load_u32(&smp->run_sleepers) != 0u) {
        smp_futex_wake_all(&smp->run_epoch);
    }
}

void smp_opt_drain(smp_opt_t *smp)
{
    DBG_CHECK(smp != NULL);

    u32 target_epoch = smp_atomic_load_u32(&smp->run_epoch);
    for (u32 spin = 0; spin < SMP_OPT_SPIN_LIMIT; spin++) {
        if (smp_atomic_load_u32(&smp->done_epoch) == target_epoch) {
            return;
        }
        cpu_relax();
    }

    if (smp_atomic_load_u32(&smp->done_epoch) != target_epoch) {
        smp_atomic_inc_u32(&smp->done_sleepers);
        while (smp_atomic_load_u32(&smp->done_epoch) != target_epoch) {
            u32 done_epoch = smp_atomic_load_u32(&smp->done_epoch);
            smp_futex_wait(&smp->done_epoch, done_epoch);
        }
        smp_atomic_dec_u32(&smp->done_sleepers);
    }
}

const u64 *smp_opt_cycle(const smp_opt_t *smp)
{
    DBG_CHECK(smp != NULL);
    return &smp->cycle_shadow;
}

#else

#include <stdlib.h>

struct smp_opt {
    u32 unused;
};

bool smp_opt_supported(void)
{
    return false;
}

const char *smp_opt_unsupported_reason(void)
{
    return "SMP optimized simulation is only supported on Linux; falling back to single-thread simulation";
}

smp_opt_t *smp_opt_create(const u64 *host_cycle,
    smp_opt_task_func_t task_func, void *task_args)
{
    (void)host_cycle;
    (void)task_func;
    (void)task_args;
    DBG_CHECK(false);
    return NULL;
}

void smp_opt_destroy(smp_opt_t *smp)
{
    (void)smp;
}

void smp_opt_drain(smp_opt_t *smp)
{
    (void)smp;
}

void smp_opt_submit(smp_opt_t *smp)
{
    (void)smp;
}

const u64 *smp_opt_cycle(const smp_opt_t *smp)
{
    (void)smp;
    DBG_CHECK(false);
    return NULL;
}

#endif
