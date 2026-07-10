#ifndef FIFO_H
#define FIFO_H

#include "types.h"
#include "dbg/chk.h"

typedef struct fifo {
    void *data;
    u32 element_size;
    u32 depth;
    u32 num;
    u32 rptr;
    u32 wptr;
} fifo_t;

extern void fifo_construct(fifo_t *f, u32 element_size, u32 depth);
extern void fifo_free(fifo_t *f);

extern void fifo_push(fifo_t *f, const void *elem);
extern void fifo_pop(fifo_t *f, void *elem);
extern void fifo_get_front(fifo_t *f, void *elem);
extern void fifo_clear(fifo_t *f);
extern void fifo_reset(fifo_t *f);

static inline bool fifo_empty(fifo_t *f)
{
    DBG_CHECK(f != NULL);
    return f->num == 0;
}

static inline bool fifo_full(fifo_t *f)
{
    DBG_CHECK(f != NULL);
    return f->num == f->depth;
}

static inline u32 fifo_count(fifo_t *f)
{
    DBG_CHECK(f != NULL);
    return f->num;
}

#endif
