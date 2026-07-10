#ifndef HART_H
#define HART_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/bti_if.h"
#include "ifu.h"
#include "exu/exu.h"
#include "csr.h"
#include "hbi.h"
#include "lsu.h"
#include "mmu.h"
#include "trap.h"
#include "l1.h"
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
    u32 ifu_ctrlq_depth;
    u32 ifu_fch_ost_depth;
    u32 hbi_stg_fifo_depth;
    u32 hbi_i_ost_depth;
    u32 hbi_d_ost_depth;
    u32 lsu_stg_fifo_depth;
    u32 lsu_ost_depth;
    u32 mmu_i_stg_fifo_depth;
    u32 mmu_d_stg_fifo_depth;
    u32 mmu_ost_depth;
    u32 l1_latency;
    u32 l1i_stg_fifo_depth;
    u32 l1d_stg_fifo_depth;
    u32 l1_ost_depth;
    u32 l1d_bti_mux_stg_fifo_depth;
    u32 l1d_bti_mux_ost_depth;
} hart_conf_t;

typedef struct hart {
    mod_t mod;
    AXI4_MST_DECL(i_);
    AXI4_MST_DECL(d_);
    itf_t *core_s_irq_slv;
    itf_t *core_timer_in;
    itf_t *core_m_irq_in;
    itf_t *ext_irq_in;
    itf_t *core_swi_pend_out;

    ifu_t ifu;
    exu_t exu;
    csr_t csr;
    lsu_t lsu;
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
    itf_t exu_lsu_ldst_req_itf;
    itf_t exu_lsu_ldst_rsp_itf;
    itf_t lsu_hbi_ldst_req_itf;
    itf_t lsu_hbi_ldst_rsp_itf;
    BTI_IF_DECL(va_i_);
    BTI_IF_DECL(va_d_);
    BTI_IF_DECL(pa_i_);
    BTI_IF_DECL(pa_d_);
    BTI_IF_DECL(pa_ptw_);
    BTI_IF_DECL(l1d_);
    itf_t tlb_flush_itf;
    itf_t l1i_flush_itf;
    itf_t mmu_fch_expt_itf;
    itf_t fch_expt_itf;
    itf_t ex_expt_itf;
    itf_t ldst_expt_itf;
    itf_t trap_send_itf;
    itf_t exu_csr_read_req_sig_itf;
    itf_t csr_exu_read_rsp_sig_itf;
    itf_t exu_csr_write_req_sig_itf;
    itf_t csr_exu_write_rsp_sig_itf;
    itf_t exu_state_sig_itf;
    itf_t trap_exu_ctrl_sig_itf;
    itf_t csr_mmu_state_sig_itf;
    itf_t csr_lsu_state_sig_itf;
    itf_t csr_trap_state_sig_itf;
    itf_t trap_csr_write_req_sig_itf;
    itf_t csr_trap_write_rsp_sig_itf;
} hart_t;

extern void hart_construct(hart_t *s, const char *parent, const char *name, const hart_conf_t *conf);
extern void hart_reset(hart_t *s);
extern void hart_clock(hart_t *s);
extern void hart_free(hart_t *s);

#endif
