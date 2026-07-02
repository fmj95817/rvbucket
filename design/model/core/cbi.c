#include "cbi.h"
#include "dbg/vcd.h"

void cbi_construct(cbi_t *cbi, const char *name, const cbi_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    AXI4_IF_CONSTRUCT(cbi, boot_rom_i_, 1);
    AXI4_IF_CONSTRUCT(cbi, boot_rom_d_, 1);
    BTI_IF_CONSTRUCT(cbi, boot_rom_i_, 1);
    BTI_IF_CONSTRUCT(cbi, boot_rom_d_, 1);
    AXI4_IF_CONSTRUCT(cbi, itcm_i_, 1);
    AXI4_IF_CONSTRUCT(cbi, itcm_d_, 1);
    AXI4_IF_CONSTRUCT(cbi, dtcm_, 1);
    AXI4_IF_CONSTRUCT(cbi, cfg_, 1);
    BTI_IF_CONSTRUCT(cbi, cfg_, 1);
    APB_IF_CONSTRUCT(cbi, cfg_, 1);

    AXI4_SLV_IMPORT(&cbi->i_axi_demux, host_, cbi, hart_i_);
    AXI4_MST_ARR_CONNECT(&cbi->i_axi_demux, gst_, 0, cbi, boot_rom_i_);
    AXI4_MST_ARR_CONNECT(&cbi->i_axi_demux, gst_, 1, cbi, itcm_i_);
    AXI4_MST_ARR_IMPORT(&cbi->i_axi_demux, gst_, 2, cbi, mm_i_);
    const u32 i_axi_gst_bases[] = { conf->boot_rom_base, conf->itcm_base, conf->mm_base };
    const u32 i_axi_gst_sizes[] = { conf->boot_rom_size, conf->itcm_size, conf->mm_size };
    axi_demux_construct(&cbi->i_axi_demux, "u_i_axi_demux", 3, i_axi_gst_bases, i_axi_gst_sizes);

    AXI4_SLV_IMPORT(&cbi->d_axi_demux, host_, cbi, hart_d_);
    AXI4_MST_ARR_CONNECT(&cbi->d_axi_demux, gst_, 0, cbi, boot_rom_d_);
    AXI4_MST_ARR_CONNECT(&cbi->d_axi_demux, gst_, 1, cbi, itcm_d_);
    AXI4_MST_ARR_CONNECT(&cbi->d_axi_demux, gst_, 2, cbi, dtcm_);
    AXI4_MST_ARR_IMPORT(&cbi->d_axi_demux, gst_, 3, cbi, mm_d_);
    AXI4_MST_ARR_CONNECT(&cbi->d_axi_demux, gst_, 4, cbi, cfg_);
    const u32 d_axi_gst_bases[] = { conf->boot_rom_base, conf->itcm_base, conf->dtcm_base, conf->mm_base, conf->cfg_base };
    const u32 d_axi_gst_sizes[] = { conf->boot_rom_size, conf->itcm_size, conf->dtcm_size, conf->mm_size, conf->cfg_size };
    axi_demux_construct(&cbi->d_axi_demux, "u_d_axi_demux", 5, d_axi_gst_bases, d_axi_gst_sizes);

    AXI4_SLV_CONNECT(&cbi->boot_rom_i_axi2bti, , cbi, boot_rom_i_);
    BTI_MST_CONNECT(&cbi->boot_rom_i_axi2bti, , cbi, boot_rom_i_);
    axi2bti_construct(&cbi->boot_rom_i_axi2bti, "u_boot_rom_i_axi2bti");

    AXI4_SLV_CONNECT(&cbi->boot_rom_d_axi2bti, , cbi, boot_rom_d_);
    BTI_MST_CONNECT(&cbi->boot_rom_d_axi2bti, , cbi, boot_rom_d_);
    axi2bti_construct(&cbi->boot_rom_d_axi2bti, "u_boot_rom_d_axi2bti");

    BTI_SLV_ARR_CONNECT(&cbi->boot_rom_bti_mux, host_, 0, cbi, boot_rom_i_);
    BTI_SLV_ARR_CONNECT(&cbi->boot_rom_bti_mux, host_, 1, cbi, boot_rom_d_);
    BTI_MST_IMPORT(&cbi->boot_rom_bti_mux, gst_, cbi, boot_rom_);
    bti_mux_construct(&cbi->boot_rom_bti_mux, "u_boot_rom_bti_mux", 2);

    AXI4_SLV_CONNECT(&cbi->itcm_i_axi2bti, , cbi, itcm_i_);
    BTI_MST_IMPORT(&cbi->itcm_i_axi2bti, , cbi, itcm_i_);
    axi2bti_construct(&cbi->itcm_i_axi2bti, "u_itcm_i_axi2bti");

    AXI4_SLV_CONNECT(&cbi->itcm_d_axi2bti, , cbi, itcm_d_);
    BTI_MST_IMPORT(&cbi->itcm_d_axi2bti, , cbi, itcm_d_);
    axi2bti_construct(&cbi->itcm_d_axi2bti, "u_itcm_d_axi2bti");

    AXI4_SLV_CONNECT(&cbi->dtcm_axi2bti, , cbi, dtcm_);
    BTI_MST_IMPORT(&cbi->dtcm_axi2bti, , cbi, dtcm_);
    axi2bti_construct(&cbi->dtcm_axi2bti, "u_dtcm_axi2bti");

    AXI4_SLV_CONNECT(&cbi->cfg_axi2bti, , cbi, cfg_);
    BTI_MST_CONNECT(&cbi->cfg_axi2bti, , cbi, cfg_);
    axi2bti_construct(&cbi->cfg_axi2bti, "u_cfg_axi2bti");

    BTI_SLV_CONNECT(&cbi->cfg_bti2apb, , cbi, cfg_);
    APB_MST_CONNECT(&cbi->cfg_bti2apb, , cbi, cfg_);
    bti2apb_construct(&cbi->cfg_bti2apb, "u_cfg_bti2apb");

    APB_SLV_CONNECT(&cbi->cfg_apb_demux, host_, cbi, cfg_);
    APB_MST_ARR_IMPORT(&cbi->cfg_apb_demux, gst_, 0, cbi, peri_);
    APB_MST_ARR_IMPORT(&cbi->cfg_apb_demux, gst_, 1, cbi, aclint_cfg_);
    APB_MST_ARR_IMPORT(&cbi->cfg_apb_demux, gst_, 2, cbi, plic_cfg_);
    const u32 cfg_apb_gst_bases[] = { conf->peri_base, conf->aclint_base, conf->plic_base };
    const u32 cfg_apb_gst_sizes[] = { conf->peri_size, conf->aclint_size, conf->plic_size };
    apb_demux_construct(&cbi->cfg_apb_demux, "u_cfg_apb_demux", 3, cfg_apb_gst_bases, cfg_apb_gst_sizes);
}

void cbi_reset(cbi_t *cbi)
{
    axi_demux_reset(&cbi->i_axi_demux);
    axi_demux_reset(&cbi->d_axi_demux);
    axi2bti_reset(&cbi->boot_rom_i_axi2bti);
    axi2bti_reset(&cbi->boot_rom_d_axi2bti);
    bti_mux_reset(&cbi->boot_rom_bti_mux);
    axi2bti_reset(&cbi->itcm_i_axi2bti);
    axi2bti_reset(&cbi->itcm_d_axi2bti);
    axi2bti_reset(&cbi->dtcm_axi2bti);
    axi2bti_reset(&cbi->cfg_axi2bti);
    bti2apb_reset(&cbi->cfg_bti2apb);
    apb_demux_reset(&cbi->cfg_apb_demux);

    AXI4_IF_RESET(cbi, boot_rom_i_);
    AXI4_IF_RESET(cbi, boot_rom_d_);
    BTI_IF_RESET(cbi, boot_rom_i_);
    BTI_IF_RESET(cbi, boot_rom_d_);
    AXI4_IF_RESET(cbi, itcm_i_);
    AXI4_IF_RESET(cbi, itcm_d_);
    AXI4_IF_RESET(cbi, dtcm_);
    AXI4_IF_RESET(cbi, cfg_);
    BTI_IF_RESET(cbi, cfg_);
    APB_IF_RESET(cbi, cfg_);
}

void cbi_free(cbi_t *cbi)
{
    axi_demux_free(&cbi->i_axi_demux);
    axi_demux_free(&cbi->d_axi_demux);
    axi2bti_free(&cbi->boot_rom_i_axi2bti);
    axi2bti_free(&cbi->boot_rom_d_axi2bti);
    bti_mux_free(&cbi->boot_rom_bti_mux);
    axi2bti_free(&cbi->itcm_i_axi2bti);
    axi2bti_free(&cbi->itcm_d_axi2bti);
    axi2bti_free(&cbi->dtcm_axi2bti);
    axi2bti_free(&cbi->cfg_axi2bti);
    bti2apb_free(&cbi->cfg_bti2apb);
    apb_demux_free(&cbi->cfg_apb_demux);

    AXI4_IF_FREE(cbi, boot_rom_i_);
    AXI4_IF_FREE(cbi, boot_rom_d_);
    BTI_IF_FREE(cbi, boot_rom_i_);
    BTI_IF_FREE(cbi, boot_rom_d_);
    AXI4_IF_FREE(cbi, itcm_i_);
    AXI4_IF_FREE(cbi, itcm_d_);
    AXI4_IF_FREE(cbi, dtcm_);
    AXI4_IF_FREE(cbi, cfg_);
    BTI_IF_FREE(cbi, cfg_);
    APB_IF_FREE(cbi, cfg_);
}

void cbi_clock(cbi_t *cbi)
{
    axi_demux_clock(&cbi->i_axi_demux);
    axi_demux_clock(&cbi->d_axi_demux);
    axi2bti_clock(&cbi->boot_rom_i_axi2bti);
    axi2bti_clock(&cbi->boot_rom_d_axi2bti);
    bti_mux_clock(&cbi->boot_rom_bti_mux);
    axi2bti_clock(&cbi->itcm_i_axi2bti);
    axi2bti_clock(&cbi->itcm_d_axi2bti);
    axi2bti_clock(&cbi->dtcm_axi2bti);
    axi2bti_clock(&cbi->cfg_axi2bti);
    bti2apb_clock(&cbi->cfg_bti2apb);
    apb_demux_clock(&cbi->cfg_apb_demux);

    AXI4_IF_DBG_CLOCK(cbi, boot_rom_i_);
    AXI4_IF_DBG_CLOCK(cbi, boot_rom_d_);
    BTI_IF_DBG_CLOCK(cbi, boot_rom_i_);
    BTI_IF_DBG_CLOCK(cbi, boot_rom_d_);
    AXI4_IF_DBG_CLOCK(cbi, itcm_i_);
    AXI4_IF_DBG_CLOCK(cbi, itcm_d_);
    AXI4_IF_DBG_CLOCK(cbi, dtcm_);
    AXI4_IF_DBG_CLOCK(cbi, cfg_);
    BTI_IF_DBG_CLOCK(cbi, cfg_);
    APB_IF_DBG_CLOCK(cbi, cfg_);
}
