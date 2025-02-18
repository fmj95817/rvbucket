#ifndef ITF_H
#define ITF_H

#include "types.h"
#include <stdio.h>

typedef void (*pkt2str_t)(const void *, char *);

typedef struct itf {
    u32 pkt_size;
    u32 fifo_depth;

    const u64 *cycle;

    bool dump_enable;
    const char *name;
    pkt2str_t pkt2str;
    FILE *dump_slv_fp;
    FILE *dump_mst_fp;

    void *pkts_data;
    u32 pkt_num;
    u32 rptr;
    u32 wptr;
} itf_t;

extern void itf_construct(itf_t *itf, const u64 *cycle, const char *name, pkt2str_t pkt2str, u32 pkt_size, u32 fifo_depth);
extern void itf_free(itf_t *itf);
extern void itf_write(itf_t *itf, const void *pkt);
extern void itf_read(itf_t *itf, void *pkt);
extern void itf_fifo_get_front(itf_t *itf, void *pkt);
extern void itf_fifo_pop_front(itf_t *itf);
extern bool itf_fifo_empty(itf_t *itf);
extern bool itf_fifo_full(itf_t *itf);

#endif