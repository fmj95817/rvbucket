#ifndef RV32G_H
#define RV32G_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "mem/rom.h"
#include "mem/ram.h"
#include "hart/hart.h"
#include "l2.h"
#include "cbi.h"
#include "aclint.h"
#include "plic.h"

typedef struct rv32g_conf {
    u32 boot_rom_base;
    u32 boot_rom_size;
    u32 itcm_base;
    u32 itcm_size;
    u32 dtcm_base;
    u32 dtcm_size;
    u32 mm_base;
    u32 mm_size;
    u32 cfg_base;
    u32 cfg_size;
    u32 peri_base;
    u32 peri_size;
    u32 aclint_base;
    u32 aclint_size;
    u32 aclint_mtimer_base;
    u32 aclint_mtimer_size;
    u32 aclint_mtimecmp_base;
    u32 aclint_mtimecmp_size;
    u32 aclint_mtimer_tick_cycles;
    u32 aclint_mswi_base;
    u32 aclint_mswi_size;
    u32 aclint_sswi_base;
    u32 aclint_sswi_size;
    u32 plic_base;
    u32 plic_size;
    u32 hart_ifu_ctrlq_depth;
    u32 hart_ifu_fch_ost_depth;
    u32 hart_hbi_stg_fifo_depth;
    u32 hart_hbi_i_ost_depth;
    u32 hart_hbi_d_ost_depth;
    u32 hart_lsu_stg_fifo_depth;
    u32 hart_lsu_ost_depth;
    u32 hart_mmu_i_stg_fifo_depth;
    u32 hart_mmu_d_stg_fifo_depth;
    u32 hart_mmu_ost_depth;
    u32 hart_l1_latency;
    u32 hart_l1i_stg_fifo_depth;
    u32 hart_l1d_stg_fifo_depth;
    u32 hart_l1_ost_depth;
    u32 hart_l1d_bti_mux_stg_fifo_depth;
    u32 hart_l1d_bti_mux_ost_depth;
    u32 cbi_bus_stg_fifo_depth;
    u32 cbi_bus_ost_depth;
    u32 l2_latency;
    u32 l2_stg_fifo_depth;
    u32 l2_bypass_ost_depth;
    u32 itcm_latency;
    u32 dtcm_latency;
} rv32g_conf_t;

typedef struct rv32g {
    mod_t mod;
    AXI4_MST_DECL(mm_);
    APB_MST_DECL(peri_);
    itf_t *ext_irq_ins[PLIC_MAX_IRQ_NUM];

    hart_t hart;
    cbi_t cbi;
    rom_t boot_rom;
    ram_t itcm;
    ram_t dtcm;
    aclint_t aclint;
    plic_t plic;
    l2_t l2;

    BTI_IF_DECL(boot_rom_);
    BTI_IF_DECL(itcm_i_);
    BTI_IF_DECL(itcm_d_);
    BTI_IF_DECL(dtcm_);
    APB_IF_DECL(aclint_cfg_);
    APB_IF_DECL(plic_cfg_);
    itf_t core_timer_sig_itf;
    itf_t core_m_irq_sig_itf;
    itf_t core_swi_pend_sig_itf;
    itf_t core_s_irq_itf;
    itf_t conv_ext_irq_sig_itf;
    AXI4_IF_DECL(hart_i_);
    AXI4_IF_DECL(hart_d_);
    AXI4_IF_DECL(cbi_mm_i_);
    AXI4_IF_DECL(cbi_mm_d_);
} rv32g_t;

extern void rv32g_construct(rv32g_t *s, const char *parent, const char *name, const rv32g_conf_t *conf);
extern void rv32g_reset(rv32g_t *s);
extern void rv32g_clock(rv32g_t *s);
extern void rv32g_free(rv32g_t *s);

#endif
