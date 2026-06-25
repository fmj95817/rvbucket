#ifndef CBI_H
#define CBI_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/fch_req_if.h"
#include "itf/fch_rsp_if.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/apb_req_if.h"
#include "itf/apb_rsp_if.h"
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
} cbi_conf_t;

typedef struct cbi {
    const u64 *cycle;
    itf_t *mm_i_axi4_aw_mst;
    itf_t *mm_i_axi4_w_mst;
    itf_t *mm_i_axi4_b_slv;
    itf_t *mm_i_axi4_ar_mst;
    itf_t *mm_i_axi4_r_slv;
    itf_t *mm_d_axi4_aw_mst;
    itf_t *mm_d_axi4_w_mst;
    itf_t *mm_d_axi4_b_slv;
    itf_t *mm_d_axi4_ar_mst;
    itf_t *mm_d_axi4_r_slv;
    itf_t *peri_apb_req_mst;
    itf_t *peri_apb_rsp_slv;

    itf_t *boot_rom_bti_req_mst;
    itf_t *boot_rom_bti_rsp_slv;
    itf_t *itcm_i_bti_req_mst;
    itf_t *itcm_i_bti_rsp_slv;
    itf_t *itcm_d_bti_req_mst;
    itf_t *itcm_d_bti_rsp_slv;
    itf_t *dtcm_bti_req_mst;
    itf_t *dtcm_bti_rsp_slv;
    itf_t *aclint_cfg_apb_req_mst;
    itf_t *aclint_cfg_apb_rsp_slv;
    itf_t *plic_cfg_apb_req_mst;
    itf_t *plic_cfg_apb_rsp_slv;

    itf_t *hart_i_axi4_aw_slv;
    itf_t *hart_i_axi4_w_slv;
    itf_t *hart_i_axi4_b_mst;
    itf_t *hart_i_axi4_ar_slv;
    itf_t *hart_i_axi4_r_mst;
    itf_t *hart_d_axi4_aw_slv;
    itf_t *hart_d_axi4_w_slv;
    itf_t *hart_d_axi4_b_mst;
    itf_t *hart_d_axi4_ar_slv;
    itf_t *hart_d_axi4_r_mst;

    axi_demux_t i_axi_demux;
    axi_demux_t d_axi_demux;
    axi2bti_t boot_rom_axi2bti;
    axi2bti_t itcm_i_axi2bti;
    axi2bti_t itcm_d_axi2bti;
    axi2bti_t dtcm_axi2bti;
    axi2bti_t cfg_axi2bti;
    bti2apb_t cfg_bti2apb;
    apb_demux_t cfg_apb_demux;

    itf_t boot_rom_axi4_aw_itf;
    itf_t boot_rom_axi4_w_itf;
    itf_t boot_rom_axi4_b_itf;
    itf_t boot_rom_axi4_ar_itf;
    itf_t boot_rom_axi4_r_itf;
    itf_t itcm_i_axi4_aw_itf;
    itf_t itcm_i_axi4_w_itf;
    itf_t itcm_i_axi4_b_itf;
    itf_t itcm_i_axi4_ar_itf;
    itf_t itcm_i_axi4_r_itf;
    itf_t itcm_d_axi4_aw_itf;
    itf_t itcm_d_axi4_w_itf;
    itf_t itcm_d_axi4_b_itf;
    itf_t itcm_d_axi4_ar_itf;
    itf_t itcm_d_axi4_r_itf;
    itf_t dtcm_axi4_aw_itf;
    itf_t dtcm_axi4_w_itf;
    itf_t dtcm_axi4_b_itf;
    itf_t dtcm_axi4_ar_itf;
    itf_t dtcm_axi4_r_itf;
    itf_t cfg_axi4_aw_itf;
    itf_t cfg_axi4_w_itf;
    itf_t cfg_axi4_b_itf;
    itf_t cfg_axi4_ar_itf;
    itf_t cfg_axi4_r_itf;
    itf_t cfg_bti_req_itf;
    itf_t cfg_bti_rsp_itf;
    itf_t cfg_apb_req_itf;
    itf_t cfg_apb_rsp_itf;
} cbi_t;

extern void cbi_construct(cbi_t *cbi, const char *name, const cbi_conf_t *conf);
extern void cbi_reset(cbi_t *cbi);
extern void cbi_clock(cbi_t *cbi);
extern void cbi_free(cbi_t *cbi);

#endif
