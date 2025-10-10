#ifndef HART_H
#define HART_H

#include "base/types.h"
#include "base/itf.h"
#include "ifu.h"
#include "exu/exu.h"
#include "csr.h"
#include "hbi.h"
#include "trap.h"

typedef struct hart_conf {
    u32 boot_rom_base;
    u32 boot_rom_size;
} hart_conf_t;

typedef struct hart {
    const u64 *cycle;
    itf_t *i_bti_req_mst;
    itf_t *i_bti_rsp_slv;
    itf_t *d_bti_req_mst;
    itf_t *d_bti_rsp_slv;
    itf_t *core_irq_slv;
    itf_t *ext_irq_slv;

    rv32g_priv_t priv;
    ifu_t ifu;
    exu_t exu;
    csr_t csr;
    hbi_t hbi;
    trap_t trap;

    itf_t ex_req_itf;
    itf_t ex_rsp_itf;
    itf_t fl_req_itf;
    itf_t fch_req_itf;
    itf_t fch_rsp_itf;
    itf_t ldst_req_itf;
    itf_t ldst_rsp_itf;
    itf_t hart_expt_itf;
    itf_t trap_send_itf;
} hart_t;

extern void hart_construct(hart_t *s, const hart_conf_t *conf);
extern void hart_reset(hart_t *s);
extern void hart_clock(hart_t *s);
extern void hart_free(hart_t *s);

#endif