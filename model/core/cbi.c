#include "cbi.h"
#include "dbg/vcd.h"

#define CONSTRUCT_AXI_ITFS(module, pfx) do { \
    AXI4_AW_IF_CONSTRUCT(module, pfx##_axi4_aw_itf, 1); \
    AXI4_W_IF_CONSTRUCT(module, pfx##_axi4_w_itf, 8); \
    AXI4_B_IF_CONSTRUCT(module, pfx##_axi4_b_itf, 1); \
    AXI4_AR_IF_CONSTRUCT(module, pfx##_axi4_ar_itf, 1); \
    AXI4_R_IF_CONSTRUCT(module, pfx##_axi4_r_itf, 8); \
} while (0)

#define FREE_AXI_ITFS(obj, pfx) do { \
    itf_free(&(obj)->pfx##_axi4_aw_itf); \
    itf_free(&(obj)->pfx##_axi4_w_itf); \
    itf_free(&(obj)->pfx##_axi4_b_itf); \
    itf_free(&(obj)->pfx##_axi4_ar_itf); \
    itf_free(&(obj)->pfx##_axi4_r_itf); \
} while (0)

#define DBG_CLOCK_AXI_ITFS(obj, pfx) do { \
    itf_dbg_clock(&(obj)->pfx##_axi4_aw_itf); \
    itf_dbg_clock(&(obj)->pfx##_axi4_w_itf); \
    itf_dbg_clock(&(obj)->pfx##_axi4_b_itf); \
    itf_dbg_clock(&(obj)->pfx##_axi4_ar_itf); \
    itf_dbg_clock(&(obj)->pfx##_axi4_r_itf); \
} while (0)

#define CONNECT_AXI2BTI(obj, br, pfx, req, rsp) do { \
    (br).axi4_aw_slv = &(obj)->pfx##_axi4_aw_itf; \
    (br).axi4_w_slv = &(obj)->pfx##_axi4_w_itf; \
    (br).axi4_b_mst = &(obj)->pfx##_axi4_b_itf; \
    (br).axi4_ar_slv = &(obj)->pfx##_axi4_ar_itf; \
    (br).axi4_r_mst = &(obj)->pfx##_axi4_r_itf; \
    (br).bti_req_mst = (req); \
    (br).bti_rsp_slv = (rsp); \
} while (0)

void cbi_construct(cbi_t *cbi, const char *name, const cbi_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    CONSTRUCT_AXI_ITFS(cbi, boot_rom);
    CONSTRUCT_AXI_ITFS(cbi, itcm_i);
    CONSTRUCT_AXI_ITFS(cbi, itcm_d);
    CONSTRUCT_AXI_ITFS(cbi, dtcm);
    CONSTRUCT_AXI_ITFS(cbi, cfg);
    BTI_REQ_IF_CONSTRUCT(cbi, cfg_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(cbi, cfg_bti_rsp_itf, 1);
    APB_REQ_IF_CONSTRUCT(cbi, cfg_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(cbi, cfg_apb_rsp_itf, 1);

    cbi->i_axi_demux.host_axi4_aw_slv = cbi->hart_i_axi4_aw_slv;
    cbi->i_axi_demux.host_axi4_w_slv = cbi->hart_i_axi4_w_slv;
    cbi->i_axi_demux.host_axi4_b_mst = cbi->hart_i_axi4_b_mst;
    cbi->i_axi_demux.host_axi4_ar_slv = cbi->hart_i_axi4_ar_slv;
    cbi->i_axi_demux.host_axi4_r_mst = cbi->hart_i_axi4_r_mst;
    cbi->i_axi_demux.gst_axi4_aw_msts[0] = &cbi->boot_rom_axi4_aw_itf;
    cbi->i_axi_demux.gst_axi4_w_msts[0] = &cbi->boot_rom_axi4_w_itf;
    cbi->i_axi_demux.gst_axi4_b_slvs[0] = &cbi->boot_rom_axi4_b_itf;
    cbi->i_axi_demux.gst_axi4_ar_msts[0] = &cbi->boot_rom_axi4_ar_itf;
    cbi->i_axi_demux.gst_axi4_r_slvs[0] = &cbi->boot_rom_axi4_r_itf;
    cbi->i_axi_demux.gst_axi4_aw_msts[1] = &cbi->itcm_i_axi4_aw_itf;
    cbi->i_axi_demux.gst_axi4_w_msts[1] = &cbi->itcm_i_axi4_w_itf;
    cbi->i_axi_demux.gst_axi4_b_slvs[1] = &cbi->itcm_i_axi4_b_itf;
    cbi->i_axi_demux.gst_axi4_ar_msts[1] = &cbi->itcm_i_axi4_ar_itf;
    cbi->i_axi_demux.gst_axi4_r_slvs[1] = &cbi->itcm_i_axi4_r_itf;
    cbi->i_axi_demux.gst_axi4_aw_msts[2] = cbi->mm_i_axi4_aw_mst;
    cbi->i_axi_demux.gst_axi4_w_msts[2] = cbi->mm_i_axi4_w_mst;
    cbi->i_axi_demux.gst_axi4_b_slvs[2] = cbi->mm_i_axi4_b_slv;
    cbi->i_axi_demux.gst_axi4_ar_msts[2] = cbi->mm_i_axi4_ar_mst;
    cbi->i_axi_demux.gst_axi4_r_slvs[2] = cbi->mm_i_axi4_r_slv;
    const u32 i_axi_gst_bases[] = { conf->boot_rom_base, conf->itcm_base, conf->mm_base };
    const u32 i_axi_gst_sizes[] = { conf->boot_rom_size, conf->itcm_size, conf->mm_size };
    axi_demux_construct(&cbi->i_axi_demux, "u_i_axi_demux", 3, i_axi_gst_bases, i_axi_gst_sizes);

    cbi->d_axi_demux.host_axi4_aw_slv = cbi->hart_d_axi4_aw_slv;
    cbi->d_axi_demux.host_axi4_w_slv = cbi->hart_d_axi4_w_slv;
    cbi->d_axi_demux.host_axi4_b_mst = cbi->hart_d_axi4_b_mst;
    cbi->d_axi_demux.host_axi4_ar_slv = cbi->hart_d_axi4_ar_slv;
    cbi->d_axi_demux.host_axi4_r_mst = cbi->hart_d_axi4_r_mst;
    cbi->d_axi_demux.gst_axi4_aw_msts[0] = &cbi->itcm_d_axi4_aw_itf;
    cbi->d_axi_demux.gst_axi4_w_msts[0] = &cbi->itcm_d_axi4_w_itf;
    cbi->d_axi_demux.gst_axi4_b_slvs[0] = &cbi->itcm_d_axi4_b_itf;
    cbi->d_axi_demux.gst_axi4_ar_msts[0] = &cbi->itcm_d_axi4_ar_itf;
    cbi->d_axi_demux.gst_axi4_r_slvs[0] = &cbi->itcm_d_axi4_r_itf;
    cbi->d_axi_demux.gst_axi4_aw_msts[1] = &cbi->dtcm_axi4_aw_itf;
    cbi->d_axi_demux.gst_axi4_w_msts[1] = &cbi->dtcm_axi4_w_itf;
    cbi->d_axi_demux.gst_axi4_b_slvs[1] = &cbi->dtcm_axi4_b_itf;
    cbi->d_axi_demux.gst_axi4_ar_msts[1] = &cbi->dtcm_axi4_ar_itf;
    cbi->d_axi_demux.gst_axi4_r_slvs[1] = &cbi->dtcm_axi4_r_itf;
    cbi->d_axi_demux.gst_axi4_aw_msts[2] = cbi->mm_d_axi4_aw_mst;
    cbi->d_axi_demux.gst_axi4_w_msts[2] = cbi->mm_d_axi4_w_mst;
    cbi->d_axi_demux.gst_axi4_b_slvs[2] = cbi->mm_d_axi4_b_slv;
    cbi->d_axi_demux.gst_axi4_ar_msts[2] = cbi->mm_d_axi4_ar_mst;
    cbi->d_axi_demux.gst_axi4_r_slvs[2] = cbi->mm_d_axi4_r_slv;
    cbi->d_axi_demux.gst_axi4_aw_msts[3] = &cbi->cfg_axi4_aw_itf;
    cbi->d_axi_demux.gst_axi4_w_msts[3] = &cbi->cfg_axi4_w_itf;
    cbi->d_axi_demux.gst_axi4_b_slvs[3] = &cbi->cfg_axi4_b_itf;
    cbi->d_axi_demux.gst_axi4_ar_msts[3] = &cbi->cfg_axi4_ar_itf;
    cbi->d_axi_demux.gst_axi4_r_slvs[3] = &cbi->cfg_axi4_r_itf;
    const u32 d_axi_gst_bases[] = { conf->itcm_base, conf->dtcm_base, conf->mm_base, conf->cfg_base };
    const u32 d_axi_gst_sizes[] = { conf->itcm_size, conf->dtcm_size, conf->mm_size, conf->cfg_size };
    axi_demux_construct(&cbi->d_axi_demux, "u_d_axi_demux", 4, d_axi_gst_bases, d_axi_gst_sizes);

    CONNECT_AXI2BTI(cbi, cbi->boot_rom_axi2bti, boot_rom, cbi->boot_rom_bti_req_mst, cbi->boot_rom_bti_rsp_slv);
    axi2bti_construct(&cbi->boot_rom_axi2bti, "u_boot_rom_axi2bti");
    CONNECT_AXI2BTI(cbi, cbi->itcm_i_axi2bti, itcm_i, cbi->itcm_i_bti_req_mst, cbi->itcm_i_bti_rsp_slv);
    axi2bti_construct(&cbi->itcm_i_axi2bti, "u_itcm_i_axi2bti");
    CONNECT_AXI2BTI(cbi, cbi->itcm_d_axi2bti, itcm_d, cbi->itcm_d_bti_req_mst, cbi->itcm_d_bti_rsp_slv);
    axi2bti_construct(&cbi->itcm_d_axi2bti, "u_itcm_d_axi2bti");
    CONNECT_AXI2BTI(cbi, cbi->dtcm_axi2bti, dtcm, cbi->dtcm_bti_req_mst, cbi->dtcm_bti_rsp_slv);
    axi2bti_construct(&cbi->dtcm_axi2bti, "u_dtcm_axi2bti");
    CONNECT_AXI2BTI(cbi, cbi->cfg_axi2bti, cfg, &cbi->cfg_bti_req_itf, &cbi->cfg_bti_rsp_itf);
    axi2bti_construct(&cbi->cfg_axi2bti, "u_cfg_axi2bti");

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
    axi_demux_reset(&cbi->i_axi_demux);
    axi_demux_reset(&cbi->d_axi_demux);
    axi2bti_reset(&cbi->boot_rom_axi2bti);
    axi2bti_reset(&cbi->itcm_i_axi2bti);
    axi2bti_reset(&cbi->itcm_d_axi2bti);
    axi2bti_reset(&cbi->dtcm_axi2bti);
    axi2bti_reset(&cbi->cfg_axi2bti);
    bti2apb_reset(&cbi->cfg_bti2apb);
    apb_demux_reset(&cbi->cfg_apb_demux);
}

void cbi_free(cbi_t *cbi)
{
    axi_demux_free(&cbi->i_axi_demux);
    axi_demux_free(&cbi->d_axi_demux);
    axi2bti_free(&cbi->boot_rom_axi2bti);
    axi2bti_free(&cbi->itcm_i_axi2bti);
    axi2bti_free(&cbi->itcm_d_axi2bti);
    axi2bti_free(&cbi->dtcm_axi2bti);
    axi2bti_free(&cbi->cfg_axi2bti);
    bti2apb_free(&cbi->cfg_bti2apb);
    apb_demux_free(&cbi->cfg_apb_demux);

    FREE_AXI_ITFS(cbi, boot_rom);
    FREE_AXI_ITFS(cbi, itcm_i);
    FREE_AXI_ITFS(cbi, itcm_d);
    FREE_AXI_ITFS(cbi, dtcm);
    FREE_AXI_ITFS(cbi, cfg);
    itf_free(&cbi->cfg_bti_req_itf);
    itf_free(&cbi->cfg_bti_rsp_itf);
    itf_free(&cbi->cfg_apb_req_itf);
    itf_free(&cbi->cfg_apb_rsp_itf);
}

void cbi_clock(cbi_t *cbi)
{
    axi_demux_clock(&cbi->i_axi_demux);
    axi_demux_clock(&cbi->d_axi_demux);
    axi2bti_clock(&cbi->boot_rom_axi2bti);
    axi2bti_clock(&cbi->itcm_i_axi2bti);
    axi2bti_clock(&cbi->itcm_d_axi2bti);
    axi2bti_clock(&cbi->dtcm_axi2bti);
    axi2bti_clock(&cbi->cfg_axi2bti);
    bti2apb_clock(&cbi->cfg_bti2apb);
    apb_demux_clock(&cbi->cfg_apb_demux);

    DBG_CLOCK_AXI_ITFS(cbi, boot_rom);
    DBG_CLOCK_AXI_ITFS(cbi, itcm_i);
    DBG_CLOCK_AXI_ITFS(cbi, itcm_d);
    DBG_CLOCK_AXI_ITFS(cbi, dtcm);
    DBG_CLOCK_AXI_ITFS(cbi, cfg);
    itf_dbg_clock(&cbi->cfg_bti_req_itf);
    itf_dbg_clock(&cbi->cfg_bti_rsp_itf);
    itf_dbg_clock(&cbi->cfg_apb_req_itf);
    itf_dbg_clock(&cbi->cfg_apb_rsp_itf);
}
