#ifndef RV32G_H
#define RV32G_H

#include "base/types.h"
#include "base/itf.h"
#include "ifu.h"
#include "biu.h"
#include "exu/exu.h"
#include "csr.h"

typedef struct rv32g {
    const u64 *cycle;

    rv32g_priv_t priv;
    csr_t csr;

    ifu_t ifu;
    exu_t exu;
    biu_t biu;

    itf_t *i_bti_req_mst;
    itf_t *i_bti_rsp_slv;
    itf_t *d_bti_req_mst;
    itf_t *d_bti_rsp_slv;

    itf_t fch_req_itf;
    itf_t fch_rsp_itf;
    itf_t ex_req_itf;
    itf_t ex_rsp_itf;
    itf_t fl_req_itf;
    itf_t ldst_req_itf;
    itf_t ldst_rsp_itf;
} rv32g_t;

extern void rv32g_construct(rv32g_t *s, const u64 *cycle, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size);
extern void rv32g_reset(rv32g_t *s);
extern void rv32g_clock(rv32g_t *s);
extern void rv32g_free(rv32g_t *s);

#endif