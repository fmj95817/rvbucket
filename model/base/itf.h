#ifndef ITF_H
#define ITF_H

#include "types.h"
#include <stdio.h>

typedef void (*pkt2str_t)(const void *, char *);
typedef void (*pkt_reg_vcd_sig_t)(const void *);

typedef struct itf {
    u32 pkt_size;
    u32 fifo_depth;

    const u64 *cycle;

    bool dump_enable;
    const char *name;
    pkt2str_t pkt2str;
    FILE *dump_slv_fp;
    FILE *dump_mst_fp;

    bool trace_enable;
    pkt_reg_vcd_sig_t reg_vcd_sig;

    void *pkts_data;
    bool *pkts_pend_mask;
    u32 pkt_num;
    u32 rptr;
    u32 wptr;

    bool read_vld;
    bool write_vld;
    u64 write_cycle;
    u64 read_cycle;
    void *read_pkt;
    void *write_pkt;
} itf_t;

extern void itf_construct(itf_t *itf, const u64 *cycle, const char *name, pkt2str_t pkt2str, pkt_reg_vcd_sig_t reg_vcd_sig, u32 pkt_size, u32 fifo_depth);
extern void itf_free(itf_t *itf);
extern void itf_write(itf_t *itf, const void *pkt);
extern void itf_read(itf_t *itf, void *pkt);
extern void itf_fifo_get_front(itf_t *itf, void *pkt);
extern void itf_fifo_pop_front(itf_t *itf);
extern bool itf_fifo_empty(itf_t *itf);
extern bool itf_fifo_full(itf_t *itf);
extern void itf_dbg_clock(itf_t *itf);

#endif