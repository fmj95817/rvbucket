#ifndef ITF_H
#define ITF_H

#include <stdio.h>
#include "types.h"
#include "dbg/vcd.h"

typedef void (*pkt2str_t)(const void *, char *);
typedef void (*pkt_reg_vcd_t)(const void *, dbg_sig_type_t);
typedef void (*sig_wr_cb_t)(void *);

typedef enum itf_mode {
    ITF_MODE_SIGNAL = 0,
    ITF_MODE_FIFO,
    ITF_MODE_MAX = ITF_MODE_FIFO,
    ITF_MODE_NUM = ITF_MODE_MAX + 1
} itf_mode_t;

typedef struct itf_conf {
    const u64 *cycle;
    itf_mode_t mode;
    u32 pkt_size;
    pkt2str_t pkt2str;
    pkt_reg_vcd_t reg_vcd;
    bool force_disable_dump;
    bool ext_sig_src;
    u32 fifo_depth;
} itf_conf_t;

typedef struct signal_itf_ctx {
    bool ext_sig_src;
    void *shared_pkt_data;
    void *old_pkt_data;
    sig_wr_cb_t sig_wr_cb;
    void *sig_wr_cb_args;
    bool write_vld;
    u64 write_cycle;
    FILE *dump_fp;
} signal_itf_ctx_t;

typedef struct fifo_itf_ctx {
    u32 fifo_depth;
    void *pkts_data;
    u32 pkt_num;
    u32 rptr;
    u32 wptr;

    FILE *dump_slv_fp;
    FILE *dump_mst_fp;

    bool *pkts_pend_mask;
    bool read_vld;
    bool write_vld;
    u64 write_cycle;
    u64 read_cycle;
    void *read_pkt;
    void *write_pkt;
} fifo_itf_ctx_t;

typedef struct itf {
    const u64 *cycle;
    const char *name;
    itf_mode_t mode;
    u32 pkt_size;

    bool dump_enable;
    pkt2str_t pkt2str;
    bool vcd_enable;
    pkt_reg_vcd_t reg_vcd;

    union {
        signal_itf_ctx_t signal;
        fifo_itf_ctx_t fifo;
    } ctx;
} itf_t;

extern void itf_construct(itf_t *itf, const char *name, const itf_conf_t *conf);
extern void itf_reset(itf_t *itf);
extern void itf_free(itf_t *itf);
extern void itf_write(itf_t *itf, const void *pkt);
extern void itf_read(itf_t *itf, void *pkt);
extern void itf_dbg_clock(itf_t *itf);

extern void *itf_signal_get_src_and_chk(itf_t *itf);
extern void itf_signal_set_src(itf_t *itf, void *src);
extern void itf_signal_set_wcb(itf_t *itf, sig_wr_cb_t cb, void *args);
extern void itf_signal_write_notify(itf_t *itf);

extern void itf_fifo_get_front(itf_t *itf, void *pkt);
extern void itf_fifo_pop_front(itf_t *itf);
extern void itf_fifo_pop_all(itf_t *itf);
extern bool itf_fifo_empty(itf_t *itf);
extern bool itf_fifo_full(itf_t *itf);

#endif