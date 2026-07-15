#ifndef OST_H
#define OST_H

#include "types.h"

typedef struct ostq_entry {
    bool vld;
    bool free_pending;
    void *ctx;
} ostq_entry_t;

typedef struct ostq {
    ostq_entry_t *entries;
    u32 depth;
    u32 ctx_size;
    u32 count;
    u32 free_pending_count;
    u32 rptr;
    u32 wptr;
} ostq_t;

typedef struct ostk_entry {
    bool vld;
    bool free_pending;
    u32 key;
    u64 issue_seq;
    void *ctx;
} ostk_entry_t;

typedef struct ostk {
    ostk_entry_t *entries;
    u32 depth;
    u32 ctx_size;
    u32 count;
    u32 free_pending_count;
    u32 wptr;
    u32 rptr;
    u64 next_issue_seq;
} ostk_t;

extern void ostq_construct(ostq_t *ost, u32 ctx_size, u32 depth);
extern void ostq_reset(ostq_t *ost);
extern void ostq_clock_slow(ostq_t *ost);
extern void ostq_free(ostq_t *ost);
extern bool ostq_alloc(ostq_t *ost, const void *ctx, u32 *slot);
extern bool ostq_peek_head(ostq_t *ost, void *ctx, u32 *slot);
extern void ostq_free_head(ostq_t *ost);
extern void ostq_read_slot(ostq_t *ost, u32 slot, void *ctx);
extern void ostq_write_slot(ostq_t *ost, u32 slot, const void *ctx);
extern bool ostq_full(const ostq_t *ost);
extern bool ostq_empty(const ostq_t *ost);
extern u32 ostq_count(const ostq_t *ost);
extern bool ostq_slot_valid(const ostq_t *ost, u32 slot);

extern void ostk_construct(ostk_t *ost, u32 ctx_size, u32 depth);
extern void ostk_reset(ostk_t *ost);
extern void ostk_clock_slow(ostk_t *ost);
extern void ostk_free(ostk_t *ost);
extern bool ostk_alloc(ostk_t *ost, u32 key, const void *ctx, u32 *slot);
extern bool ostk_peek_key(ostk_t *ost, u32 key, void *ctx, u32 *slot);
extern void ostk_free_slot(ostk_t *ost, u32 slot);
extern void ostk_read_slot(ostk_t *ost, u32 slot, void *ctx);
extern void ostk_write_slot(ostk_t *ost, u32 slot, const void *ctx);
extern bool ostk_full(const ostk_t *ost);
extern bool ostk_empty(const ostk_t *ost);
extern u32 ostk_count(const ostk_t *ost);
extern bool ostk_slot_valid(const ostk_t *ost, u32 slot);

static inline void ostq_clock(ostq_t *ost)
{
    if (ost->free_pending_count != 0) {
        ostq_clock_slow(ost);
    }
}

static inline void ostk_clock(ostk_t *ost)
{
    if (ost->free_pending_count != 0) {
        ostk_clock_slow(ost);
    }
}

#endif
