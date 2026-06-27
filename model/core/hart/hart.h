#ifndef HART_H
#define HART_H

#include "base/types.h"
#include "base/itf.h"
#include "ifu.h"
#include "exu/exu.h"
#include "csr.h"
#include "hbi.h"
#include "mmu.h"
#include "trap.h"
#include "mem/l1.h"
#include "bus/mux.h"

typedef struct hart_conf {
    u32 boot_rom_base;
    u32 boot_rom_size;
    u32 itcm_base;
    u32 itcm_size;
    u32 dtcm_base;
    u32 dtcm_size;
    u32 cfg_base;
    u32 cfg_size;
} hart_conf_t;

typedef struct hart {
    const u64 *cycle;
    itf_t *i_axi4_aw_mst;
    itf_t *i_axi4_w_mst;
    itf_t *i_axi4_b_slv;
    itf_t *i_axi4_ar_mst;
    itf_t *i_axi4_r_slv;
    itf_t *d_axi4_aw_mst;
    itf_t *d_axi4_w_mst;
    itf_t *d_axi4_b_slv;
    itf_t *d_axi4_ar_mst;
    itf_t *d_axi4_r_slv;
    itf_t *core_s_irq_slv;
    itf_t *core_timer_in;
    itf_t *core_m_irq_in;
    itf_t *ext_irq_in;
    itf_t *core_swi_pend_out;

    ifu_t ifu;
    exu_t exu;
    csr_t csr;
    hbi_t hbi;
    mmu_t mmu;
    l1_t l1i;
    l1_t l1d;
    bti_mux_t l1d_bti_mux;
    trap_t trap;

    itf_t ex_req_itf;
    itf_t ex_rsp_itf;
    itf_t fl_req_itf;
    itf_t fch_req_itf;
    itf_t fch_rsp_itf;
    itf_t ldst_req_itf;
    itf_t ldst_rsp_itf;
    itf_t va_i_bti_req_itf;
    itf_t va_i_bti_rsp_itf;
    itf_t va_d_bti_req_itf;
    itf_t va_d_bti_rsp_itf;
    itf_t pa_i_bti_req_itf;
    itf_t pa_i_bti_rsp_itf;
    itf_t pa_d_bti_req_itf;
    itf_t pa_d_bti_rsp_itf;
    itf_t pa_ptw_bti_req_itf;
    itf_t pa_ptw_bti_rsp_itf;
    itf_t l1d_bti_req_itf;
    itf_t l1d_bti_rsp_itf;
    itf_t tlb_flush_itf;
    itf_t l1i_flush_itf;
    itf_t i_hart_expt_itf;
    itf_t hart_expt_itf;
    itf_t trap_send_itf;
    itf_t exu_csr_read_req_sig_itf;
    itf_t csr_exu_read_rsp_sig_itf;
    itf_t exu_csr_write_req_sig_itf;
    itf_t csr_exu_write_rsp_sig_itf;
} hart_t;

extern void hart_construct(hart_t *s, const char *name, const hart_conf_t *conf);
extern void hart_reset(hart_t *s);
extern void hart_clock(hart_t *s);
extern void hart_free(hart_t *s);

#endif
