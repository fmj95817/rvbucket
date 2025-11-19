#include "cbi.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void cbi_construct(cbi_t *cbi, const char *name, const cbi_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    BTI_REQ_IF_CONSTRUCT(cbi, cfg_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(cbi, cfg_bti_rsp_itf, 1);
    APB_REQ_IF_CONSTRUCT(cbi, cfg_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(cbi, cfg_apb_rsp_itf, 1);

    cbi->i_bti_demux.host_bti_req_slv = cbi->hart_i_bti_req_slv;
    cbi->i_bti_demux.host_bti_rsp_mst = cbi->hart_i_bti_rsp_mst;
    cbi->i_bti_demux.gst_bti_req_msts[0] = cbi->boot_rom_bti_req_mst;
    cbi->i_bti_demux.gst_bti_rsp_slvs[0] = cbi->boot_rom_bti_rsp_slv;
    cbi->i_bti_demux.gst_bti_req_msts[1] = cbi->itcm_i_bti_req_mst;
    cbi->i_bti_demux.gst_bti_rsp_slvs[1] = cbi->itcm_i_bti_rsp_slv;
    cbi->i_bti_demux.gst_bti_req_msts[2] = cbi->mm_i_bti_req_mst;
    cbi->i_bti_demux.gst_bti_rsp_slvs[2] = cbi->mm_i_bti_rsp_slv;
    const u32 i_bti_gst_bases[] = { conf->boot_rom_base, conf->itcm_base, conf->mm_base };
    const u32 i_bti_gst_sizes[] = { conf->boot_rom_size, conf->itcm_size, conf->mm_size };
    bti_demux_construct(&cbi->i_bti_demux, "u_i_bti_demux", 3, i_bti_gst_bases, i_bti_gst_sizes);

    cbi->d_bti_demux.host_bti_req_slv = cbi->hart_d_bti_req_slv;
    cbi->d_bti_demux.host_bti_rsp_mst = cbi->hart_d_bti_rsp_mst;
    cbi->d_bti_demux.gst_bti_req_msts[0] = cbi->itcm_d_bti_req_mst;
    cbi->d_bti_demux.gst_bti_rsp_slvs[0] = cbi->itcm_d_bti_rsp_slv;
    cbi->d_bti_demux.gst_bti_req_msts[1] = cbi->dtcm_bti_req_mst;
    cbi->d_bti_demux.gst_bti_rsp_slvs[1] = cbi->dtcm_bti_rsp_slv;
    cbi->d_bti_demux.gst_bti_req_msts[2] = cbi->mm_d_bti_req_mst;
    cbi->d_bti_demux.gst_bti_rsp_slvs[2] = cbi->mm_d_bti_rsp_slv;
    cbi->d_bti_demux.gst_bti_req_msts[3] = &cbi->cfg_bti_req_itf;
    cbi->d_bti_demux.gst_bti_rsp_slvs[3] = &cbi->cfg_bti_rsp_itf;
    const u32 d_bti_gst_bases[] = { conf->itcm_base, conf->dtcm_base, conf->mm_base, conf->cfg_base };
    const u32 d_bti_gst_sizes[] = { conf->itcm_size, conf->dtcm_size, conf->mm_size, conf->cfg_size };
    bti_demux_construct(&cbi->d_bti_demux, "u_d_bti_demux", 4, d_bti_gst_bases, d_bti_gst_sizes);

    cbi->cfg_bti2apb.bti_req_slv = &cbi->cfg_bti_req_itf;
    cbi->cfg_bti2apb.bti_rsp_mst = &cbi->cfg_bti_rsp_itf;
    cbi->cfg_bti2apb.apb_req_mst = &cbi->cfg_apb_req_itf;
    cbi->cfg_bti2apb.apb_rsp_slv = &cbi->cfg_apb_rsp_itf;
    bti2apb_construct(&cbi->cfg_bti2apb, "u_cfg_bti2apb");

    cbi->cfg_apb_demux.host_apb_req_slv = &cbi->cfg_apb_req_itf;
    cbi->cfg_apb_demux.host_apb_rsp_mst = &cbi->cfg_apb_rsp_itf;
    cbi->cfg_apb_demux.gst_apb_req_msts[0] = cbi->peri_apb_req_mst;
    cbi->cfg_apb_demux.gst_apb_rsp_slvs[0] = cbi->peri_apb_rsp_slv;
    cbi->cfg_apb_demux.gst_apb_req_msts[1] = cbi->aclint_cfg_apb_req_mst;
    cbi->cfg_apb_demux.gst_apb_rsp_slvs[1] = cbi->aclint_cfg_apb_rsp_slv;
    cbi->cfg_apb_demux.gst_apb_req_msts[2] = cbi->plic_cfg_apb_req_mst;
    cbi->cfg_apb_demux.gst_apb_rsp_slvs[2] = cbi->plic_cfg_apb_rsp_slv;
    const u32 cfg_apb_gst_bases[] = { conf->peri_base, conf->aclint_base, conf->plic_base };
    const u32 cfg_apb_gst_sizes[] = { conf->peri_size, conf->aclint_size, conf->plic_size };
    apb_demux_construct(&cbi->cfg_apb_demux, "u_cfg_apb_demux", 3, cfg_apb_gst_bases, cfg_apb_gst_sizes);
}

void cbi_reset(cbi_t *cbi)
{
    bti_demux_reset(&cbi->i_bti_demux);
    bti_demux_reset(&cbi->d_bti_demux);
    bti2apb_reset(&cbi->cfg_bti2apb);
    apb_demux_reset(&cbi->cfg_apb_demux);
}

void cbi_free(cbi_t *cbi)
{
    bti_demux_free(&cbi->i_bti_demux);
    bti_demux_free(&cbi->d_bti_demux);
    bti2apb_free(&cbi->cfg_bti2apb);
    apb_demux_free(&cbi->cfg_apb_demux);

    itf_free(&cbi->cfg_bti_req_itf);
    itf_free(&cbi->cfg_bti_rsp_itf);
    itf_free(&cbi->cfg_apb_req_itf);
    itf_free(&cbi->cfg_apb_rsp_itf);
}

void cbi_clock(cbi_t *cbi)
{
    bti_demux_clock(&cbi->i_bti_demux);
    bti_demux_clock(&cbi->d_bti_demux);
    bti2apb_clock(&cbi->cfg_bti2apb);
    apb_demux_clock(&cbi->cfg_apb_demux);

    itf_dbg_clock(&cbi->cfg_bti_req_itf);
    itf_dbg_clock(&cbi->cfg_bti_rsp_itf);
    itf_dbg_clock(&cbi->cfg_apb_req_itf);
    itf_dbg_clock(&cbi->cfg_apb_rsp_itf);
}