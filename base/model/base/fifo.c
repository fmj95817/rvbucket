#include "fifo.h"
#include <stdlib.h>
#include <string.h>
#include "dbg/chk.h"

void fifo_construct(fifo_t *f, u32 element_size, u32 depth)
{
    DBG_CHECK(element_size > 0);
    DBG_CHECK(depth > 0);

    f->element_size = element_size;
    f->depth = depth;
    f->num = 0;
    f->rptr = 0;
    f->wptr = 0;
    f->data = malloc(element_size * depth);
    DBG_CHECK(f->data != NULL);
}

void fifo_free(fifo_t *f)
{
    if (f->data) {
        free(f->data);
        f->data = NULL;
    }
}

static inline void *fifo_elem_addr(fifo_t *f, u32 idx)
{
    return (u8 *)f->data + idx * f->element_size;
}

void fifo_push(fifo_t *f, const void *elem)
{
    DBG_CHECK(f->num < f->depth);
    DBG_CHECK(elem != NULL);

    memcpy(fifo_elem_addr(f, f->wptr), elem, f->element_size);
    f->num++;
    f->wptr = (f->wptr + 1) % f->depth;
}

void fifo_pop(fifo_t *f, void *elem)
{
    DBG_CHECK(f->num > 0);
    DBG_CHECK(elem != NULL);

    memcpy(elem, fifo_elem_addr(f, f->rptr), f->element_size);
    f->num--;
    f->rptr = (f->rptr + 1) % f->depth;
}

void fifo_get_front(fifo_t *f, void *elem)
{
    DBG_CHECK(f->num > 0);
    DBG_CHECK(elem != NULL);

    memcpy(elem, fifo_elem_addr(f, f->rptr), f->element_size);
}

void fifo_clear(fifo_t *f)
{
    f->num = 0;
    f->rptr = f->wptr;
}

void fifo_reset(fifo_t *f)
{
    f->num = 0;
    f->rptr = 0;
    f->wptr = 0;
}
