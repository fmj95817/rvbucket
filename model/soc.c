#include "soc.h"
#include "base/def.h"
#include "spec/core/core.h"
#include "spec/soc.h"
#include "itf/ext_irq_if.h"
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

void soc_construct(soc_t *soc, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    CONSTRUCT_AXI_ITFS(soc, mm_i);
    CONSTRUCT_AXI_ITFS(soc, mm_d);
    CONSTRUCT_AXI_ITFS(soc, ddr_i);
    CONSTRUCT_AXI_ITFS(soc, ddr_d);
    CONSTRUCT_AXI_ITFS(soc, flash_i);
    CONSTRUCT_AXI_ITFS(soc, flash_d);
    CONSTRUCT_AXI_ITFS(soc, flash);
    APB_REQ_IF_CONSTRUCT(soc, peri_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(soc, peri_apb_rsp_itf, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(soc, peri_irq_sig_itf, false, false);

    soc->cpu.cycle = soc->cycle;
    soc->cpu.mm_i_axi4_aw_mst = &soc->mm_i_axi4_aw_itf;
    soc->cpu.mm_i_axi4_w_mst = &soc->mm_i_axi4_w_itf;
    soc->cpu.mm_i_axi4_b_slv = &soc->mm_i_axi4_b_itf;
    soc->cpu.mm_i_axi4_ar_mst = &soc->mm_i_axi4_ar_itf;
    soc->cpu.mm_i_axi4_r_slv = &soc->mm_i_axi4_r_itf;
    soc->cpu.mm_d_axi4_aw_mst = &soc->mm_d_axi4_aw_itf;
    soc->cpu.mm_d_axi4_w_mst = &soc->mm_d_axi4_w_itf;
    soc->cpu.mm_d_axi4_b_slv = &soc->mm_d_axi4_b_itf;
    soc->cpu.mm_d_axi4_ar_mst = &soc->mm_d_axi4_ar_itf;
    soc->cpu.mm_d_axi4_r_slv = &soc->mm_d_axi4_r_itf;
    soc->cpu.peri_apb_req_mst = &soc->peri_apb_req_itf;
    soc->cpu.peri_apb_rsp_slv = &soc->peri_apb_rsp_itf;
    soc->cpu.ext_irq_ins[0] = &soc->peri_irq_sig_itf;
    for (u32 i = 1; i < PLIC_MAX_IRQ_NUM; i++) {
        soc->cpu.ext_irq_ins[i] = soc->ext_irq_ins[i];
    }
    rv32g_conf_t rv32g_conf = {
        .boot_rom_base = BOOT_ROM_BASE,
        .boot_rom_size = BOOT_ROM_SIZE,
        .itcm_base = ITCM_BASE,
        .itcm_size = ITCM_SIZE,
        .dtcm_base = DTCM_BASE,
        .dtcm_size = DTCM_SIZE,
        .mm_base = MM_BASE,
        .mm_size = MM_SIZE,
        .cfg_base = CFG_BASE,
        .cfg_size = CFG_SIZE,
        .peri_base = PERI_BASE,
        .peri_size = PERI_SIZE,
        .aclint_base = ACLINT_BASE,
        .aclint_size = ACLINT_SIZE,
        .aclint_mtimer_base = ACLINT_MTIMER_BASE,
        .aclint_mtimer_size = ACLINT_MTIMER_SIZE,
        .aclint_mtimecmp_base = ACLINT_MTIMECMP_BASE,
        .aclint_mtimecmp_size = ACLINT_MTIMECMP_SIZE,
        .aclint_mtimer_tick_cycles = ACLINT_MTIMER_TICK_CYCLES,
        .aclint_mswi_base = ACLINT_MSWI_BASE,
        .aclint_mswi_size = ACLINT_MSWI_SIZE,
        .aclint_sswi_base = ACLINT_SSWI_BASE,
        .aclint_sswi_size = ACLINT_SSWI_SIZE,
        .plic_base = PLIC_BASE,
        .plic_size = PLIC_SIZE
    };
    rv32g_construct(&soc->cpu, "u_rv32g_cpu", &rv32g_conf);

    soc->flash.axi4_aw_slv = &soc->flash_axi4_aw_itf;
    soc->flash.axi4_w_slv = &soc->flash_axi4_w_itf;
    soc->flash.axi4_b_mst = &soc->flash_axi4_b_itf;
    soc->flash.axi4_ar_slv = &soc->flash_axi4_ar_itf;
    soc->flash.axi4_r_mst = &soc->flash_axi4_r_itf;
    rom_construct(&soc->flash, "u_flash", ROM_MODE_AXI, FLASH_SIZE, NULL, 0, FLASH_BASE);

    soc->peri.cycle = soc->cycle;
    soc->peri.apb_req_slv = &soc->peri_apb_req_itf;
    soc->peri.apb_rsp_mst = &soc->peri_apb_rsp_itf;
    soc->peri.uart_tx_mst = soc->uart_tx_mst;
    soc->peri.uart_rx_slv = soc->uart_rx_slv;
    soc->peri.irq_out = &soc->peri_irq_sig_itf;
    soc->peri.gpio_out = soc->gpio_out;
    peri_construct(&soc->peri, "u_peri", PERI_BASE, PERI_SIZE);

    soc->mm_i_axi_demux.host_axi4_aw_slv = &soc->mm_i_axi4_aw_itf;
    soc->mm_i_axi_demux.host_axi4_w_slv = &soc->mm_i_axi4_w_itf;
    soc->mm_i_axi_demux.host_axi4_b_mst = &soc->mm_i_axi4_b_itf;
    soc->mm_i_axi_demux.host_axi4_ar_slv = &soc->mm_i_axi4_ar_itf;
    soc->mm_i_axi_demux.host_axi4_r_mst = &soc->mm_i_axi4_r_itf;
    soc->mm_i_axi_demux.gst_axi4_aw_msts[0] = &soc->ddr_i_axi4_aw_itf;
    soc->mm_i_axi_demux.gst_axi4_w_msts[0] = &soc->ddr_i_axi4_w_itf;
    soc->mm_i_axi_demux.gst_axi4_b_slvs[0] = &soc->ddr_i_axi4_b_itf;
    soc->mm_i_axi_demux.gst_axi4_ar_msts[0] = &soc->ddr_i_axi4_ar_itf;
    soc->mm_i_axi_demux.gst_axi4_r_slvs[0] = &soc->ddr_i_axi4_r_itf;
    soc->mm_i_axi_demux.gst_axi4_aw_msts[1] = &soc->flash_i_axi4_aw_itf;
    soc->mm_i_axi_demux.gst_axi4_w_msts[1] = &soc->flash_i_axi4_w_itf;
    soc->mm_i_axi_demux.gst_axi4_b_slvs[1] = &soc->flash_i_axi4_b_itf;
    soc->mm_i_axi_demux.gst_axi4_ar_msts[1] = &soc->flash_i_axi4_ar_itf;
    soc->mm_i_axi_demux.gst_axi4_r_slvs[1] = &soc->flash_i_axi4_r_itf;
    const u32 mm_axi_gst_bases[] = { DDR_BASE, FLASH_BASE };
    const u32 mm_axi_gst_sizes[] = { DDR_SIZE, FLASH_SIZE };
    axi_demux_construct(&soc->mm_i_axi_demux, "u_mm_i_axi_demux", 2, mm_axi_gst_bases, mm_axi_gst_sizes);

    soc->mm_d_axi_demux.host_axi4_aw_slv = &soc->mm_d_axi4_aw_itf;
    soc->mm_d_axi_demux.host_axi4_w_slv = &soc->mm_d_axi4_w_itf;
    soc->mm_d_axi_demux.host_axi4_b_mst = &soc->mm_d_axi4_b_itf;
    soc->mm_d_axi_demux.host_axi4_ar_slv = &soc->mm_d_axi4_ar_itf;
    soc->mm_d_axi_demux.host_axi4_r_mst = &soc->mm_d_axi4_r_itf;
    soc->mm_d_axi_demux.gst_axi4_aw_msts[0] = &soc->ddr_d_axi4_aw_itf;
    soc->mm_d_axi_demux.gst_axi4_w_msts[0] = &soc->ddr_d_axi4_w_itf;
    soc->mm_d_axi_demux.gst_axi4_b_slvs[0] = &soc->ddr_d_axi4_b_itf;
    soc->mm_d_axi_demux.gst_axi4_ar_msts[0] = &soc->ddr_d_axi4_ar_itf;
    soc->mm_d_axi_demux.gst_axi4_r_slvs[0] = &soc->ddr_d_axi4_r_itf;
    soc->mm_d_axi_demux.gst_axi4_aw_msts[1] = &soc->flash_d_axi4_aw_itf;
    soc->mm_d_axi_demux.gst_axi4_w_msts[1] = &soc->flash_d_axi4_w_itf;
    soc->mm_d_axi_demux.gst_axi4_b_slvs[1] = &soc->flash_d_axi4_b_itf;
    soc->mm_d_axi_demux.gst_axi4_ar_msts[1] = &soc->flash_d_axi4_ar_itf;
    soc->mm_d_axi_demux.gst_axi4_r_slvs[1] = &soc->flash_d_axi4_r_itf;
    axi_demux_construct(&soc->mm_d_axi_demux, "u_mm_d_axi_demux", 2, mm_axi_gst_bases, mm_axi_gst_sizes);

    soc->ddr_axi_mux.host_axi4_aw_slvs[0] = &soc->ddr_i_axi4_aw_itf;
    soc->ddr_axi_mux.host_axi4_w_slvs[0] = &soc->ddr_i_axi4_w_itf;
    soc->ddr_axi_mux.host_axi4_b_msts[0] = &soc->ddr_i_axi4_b_itf;
    soc->ddr_axi_mux.host_axi4_ar_slvs[0] = &soc->ddr_i_axi4_ar_itf;
    soc->ddr_axi_mux.host_axi4_r_msts[0] = &soc->ddr_i_axi4_r_itf;
    soc->ddr_axi_mux.host_axi4_aw_slvs[1] = &soc->ddr_d_axi4_aw_itf;
    soc->ddr_axi_mux.host_axi4_w_slvs[1] = &soc->ddr_d_axi4_w_itf;
    soc->ddr_axi_mux.host_axi4_b_msts[1] = &soc->ddr_d_axi4_b_itf;
    soc->ddr_axi_mux.host_axi4_ar_slvs[1] = &soc->ddr_d_axi4_ar_itf;
    soc->ddr_axi_mux.host_axi4_r_msts[1] = &soc->ddr_d_axi4_r_itf;
    soc->ddr_axi_mux.gst_axi4_aw_mst = soc->ddr_axi4_aw_mst;
    soc->ddr_axi_mux.gst_axi4_w_mst = soc->ddr_axi4_w_mst;
    soc->ddr_axi_mux.gst_axi4_b_slv = soc->ddr_axi4_b_slv;
    soc->ddr_axi_mux.gst_axi4_ar_mst = soc->ddr_axi4_ar_mst;
    soc->ddr_axi_mux.gst_axi4_r_slv = soc->ddr_axi4_r_slv;
    axi_mux_construct(&soc->ddr_axi_mux, "u_ddr_axi_mux", 2);

    soc->flash_axi_mux.host_axi4_aw_slvs[0] = &soc->flash_i_axi4_aw_itf;
    soc->flash_axi_mux.host_axi4_w_slvs[0] = &soc->flash_i_axi4_w_itf;
    soc->flash_axi_mux.host_axi4_b_msts[0] = &soc->flash_i_axi4_b_itf;
    soc->flash_axi_mux.host_axi4_ar_slvs[0] = &soc->flash_i_axi4_ar_itf;
    soc->flash_axi_mux.host_axi4_r_msts[0] = &soc->flash_i_axi4_r_itf;
    soc->flash_axi_mux.host_axi4_aw_slvs[1] = &soc->flash_d_axi4_aw_itf;
    soc->flash_axi_mux.host_axi4_w_slvs[1] = &soc->flash_d_axi4_w_itf;
    soc->flash_axi_mux.host_axi4_b_msts[1] = &soc->flash_d_axi4_b_itf;
    soc->flash_axi_mux.host_axi4_ar_slvs[1] = &soc->flash_d_axi4_ar_itf;
    soc->flash_axi_mux.host_axi4_r_msts[1] = &soc->flash_d_axi4_r_itf;
    soc->flash_axi_mux.gst_axi4_aw_mst = &soc->flash_axi4_aw_itf;
    soc->flash_axi_mux.gst_axi4_w_mst = &soc->flash_axi4_w_itf;
    soc->flash_axi_mux.gst_axi4_b_slv = &soc->flash_axi4_b_itf;
    soc->flash_axi_mux.gst_axi4_ar_mst = &soc->flash_axi4_ar_itf;
    soc->flash_axi_mux.gst_axi4_r_slv = &soc->flash_axi4_r_itf;
    axi_mux_construct(&soc->flash_axi_mux, "u_flash_axi_mux", 2);
}

void soc_reset(soc_t *soc)
{
    rv32g_reset(&soc->cpu);
    rom_reset(&soc->flash);
    peri_reset(&soc->peri);
    axi_demux_reset(&soc->mm_i_axi_demux);
    axi_demux_reset(&soc->mm_d_axi_demux);
    axi_mux_reset(&soc->ddr_axi_mux);
    axi_mux_reset(&soc->flash_axi_mux);
}

void soc_clock(soc_t *soc)
{
    rv32g_clock(&soc->cpu);
    rom_clock(&soc->flash);
    peri_clock(&soc->peri);
    axi_demux_clock(&soc->mm_i_axi_demux);
    axi_demux_clock(&soc->mm_d_axi_demux);
    axi_mux_clock(&soc->ddr_axi_mux);
    axi_mux_clock(&soc->flash_axi_mux);

    DBG_CLOCK_AXI_ITFS(soc, mm_i);
    DBG_CLOCK_AXI_ITFS(soc, mm_d);
    DBG_CLOCK_AXI_ITFS(soc, ddr_i);
    DBG_CLOCK_AXI_ITFS(soc, ddr_d);
    DBG_CLOCK_AXI_ITFS(soc, flash_i);
    DBG_CLOCK_AXI_ITFS(soc, flash_d);
    DBG_CLOCK_AXI_ITFS(soc, flash);
    itf_dbg_clock(&soc->peri_apb_req_itf);
    itf_dbg_clock(&soc->peri_apb_rsp_itf);
    itf_dbg_clock(&soc->peri_irq_sig_itf);
}

void soc_free(soc_t *soc)
{
    rv32g_free(&soc->cpu);
    rom_free(&soc->flash);
    peri_free(&soc->peri);
    axi_demux_free(&soc->mm_i_axi_demux);
    axi_demux_free(&soc->mm_d_axi_demux);
    axi_mux_free(&soc->ddr_axi_mux);
    axi_mux_free(&soc->flash_axi_mux);

    FREE_AXI_ITFS(soc, mm_i);
    FREE_AXI_ITFS(soc, mm_d);
    FREE_AXI_ITFS(soc, ddr_i);
    FREE_AXI_ITFS(soc, ddr_d);
    FREE_AXI_ITFS(soc, flash_i);
    FREE_AXI_ITFS(soc, flash_d);
    FREE_AXI_ITFS(soc, flash);
    itf_free(&soc->peri_apb_req_itf);
    itf_free(&soc->peri_apb_rsp_itf);
}
