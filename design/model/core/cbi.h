#ifndef CBI_H
#define CBI_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/fch_req_if.h"
#include "itf/fch_rsp_if.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/bti_if.h"
#include "itf/apb_if.h"
#include "bus/mux.h"
#include "bus/demux.h"
#include "bus/bridge.h"

typedef struct cbi_conf {
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
    u32 plic_base;
    u32 plic_size;
    u32 bus_stg_fifo_depth;
    u32 bus_ost_depth;
} cbi_conf_t;

typedef struct cbi {
    mod_t mod;
    AXI4_MST_DECL(mm_i_);
    AXI4_MST_DECL(mm_d_);
    APB_MST_DECL(peri_);
    BTI_MST_DECL(boot_rom_);
    BTI_MST_DECL(itcm_i_);
    BTI_MST_DECL(itcm_d_);
    BTI_MST_DECL(dtcm_);
    APB_MST_DECL(aclint_cfg_);
    APB_MST_DECL(plic_cfg_);
    AXI4_SLV_DECL(hart_i_);
    AXI4_SLV_DECL(hart_d_);

    axi_demux_t i_axi_demux;
    axi_demux_t d_axi_demux;
    axi2bti_t boot_rom_i_axi2bti;
    axi2bti_t boot_rom_d_axi2bti;
    bti_mux_t boot_rom_bti_mux;
    axi2bti_t itcm_i_axi2bti;
    axi2bti_t itcm_d_axi2bti;
    axi2bti_t dtcm_axi2bti;
    axi2bti_t cfg_axi2bti;
    bti2apb_t cfg_bti2apb;
    apb_demux_t cfg_apb_demux;

    AXI4_IF_DECL(boot_rom_i_);
    AXI4_IF_DECL(boot_rom_d_);
    BTI_IF_DECL(boot_rom_i_);
    BTI_IF_DECL(boot_rom_d_);
    AXI4_IF_DECL(itcm_i_);
    AXI4_IF_DECL(itcm_d_);
    AXI4_IF_DECL(dtcm_);
    AXI4_IF_DECL(cfg_);
    BTI_IF_DECL(cfg_);
    APB_IF_DECL(cfg_);
} cbi_t;

extern void cbi_construct(cbi_t *cbi, const char *parent, const char *name, const cbi_conf_t *conf);
extern void cbi_reset(cbi_t *cbi);
extern void cbi_clock(cbi_t *cbi);
extern void cbi_free(cbi_t *cbi);

#endif
