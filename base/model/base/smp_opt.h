#ifndef SMP_OPT_H
#define SMP_OPT_H

#include "types.h"

typedef void (*smp_opt_task_func_t)(void *);

typedef struct smp_opt smp_opt_t;

typedef struct smp_opt_lock {
    u32 val;
} smp_opt_lock_t;

/* Even means stable; odd means a writer is updating a protected snapshot. */
typedef struct smp_opt_snapshot_seq {
    u32 val;
} smp_opt_snapshot_seq_t;

extern bool smp_opt_supported(void);
extern const char *smp_opt_unsupported_reason(void);
extern smp_opt_t *smp_opt_create(const u64 *host_cycle,
    smp_opt_task_func_t task_func, void *task_args);
extern void smp_opt_destroy(smp_opt_t *smp);
extern void smp_opt_drain(smp_opt_t *smp);
extern void smp_opt_submit(smp_opt_t *smp);
extern const u64 *smp_opt_cycle(const smp_opt_t *smp);

extern void smp_opt_lock_init(smp_opt_lock_t *lock);
extern void smp_opt_lock_acquire(smp_opt_lock_t *lock);
extern void smp_opt_lock_release(smp_opt_lock_t *lock);

extern void smp_opt_snapshot_seq_init(smp_opt_snapshot_seq_t *seq);
extern u32 smp_opt_snapshot_seq_write_begin(smp_opt_snapshot_seq_t *seq);
extern void smp_opt_snapshot_seq_write_end(smp_opt_snapshot_seq_t *seq, u32 seq0);
extern u32 smp_opt_snapshot_seq_read_begin(smp_opt_snapshot_seq_t *seq);
extern bool smp_opt_snapshot_seq_read_retry(smp_opt_snapshot_seq_t *seq, u32 seq0);

#endif
