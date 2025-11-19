#include "soc.h"
#include "base/def.h"
#include "conf/core.h"
#include "conf/soc.h"
#include "dbg/vcd.h"

void soc_construct(soc_t *soc)
{
    itf_construct(&soc->mm_i_bti_req_itf, soc->cycle, "mm_i_bti_req_itf", &bti_req_if_to_str, &bti_req_if_reg_vcd_sig, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->mm_i_bti_rsp_itf, soc->cycle, "mm_i_bti_rsp_itf", &bti_rsp_if_to_str, &bti_rsp_if_reg_vcd_sig, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->mm_d_bti_req_itf, soc->cycle, "mm_d_bti_req_itf", &bti_req_if_to_str, &bti_req_if_reg_vcd_sig, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->mm_d_bti_rsp_itf, soc->cycle, "mm_d_bti_rsp_itf", &bti_rsp_if_to_str, &bti_rsp_if_reg_vcd_sig, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->ddr_d_bti_req_itf, soc->cycle, "ddr_d_bti_req_itf", &bti_req_if_to_str, &bti_req_if_reg_vcd_sig, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->ddr_d_bti_rsp_itf, soc->cycle, "ddr_d_bti_rsp_itf", &bti_rsp_if_to_str, &bti_rsp_if_reg_vcd_sig, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->flash_bti_req_itf, soc->cycle, "flash_bti_req_itf", &bti_req_if_to_str, &bti_req_if_reg_vcd_sig, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->flash_bti_rsp_itf, soc->cycle, "flash_bti_rsp_itf", &bti_rsp_if_to_str, &bti_rsp_if_reg_vcd_sig, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->uart_apb_req_itf, soc->cycle, "uart_apb_req_itf", &apb_req_if_to_str, &apb_req_if_reg_vcd_sig, sizeof(apb_req_if_t), 1);
    itf_construct(&soc->uart_apb_rsp_itf, soc->cycle, "uart_apb_rsp_itf", &apb_rsp_if_to_str, &apb_rsp_if_reg_vcd_sig, sizeof(apb_rsp_if_t), 1);

    soc->cpu.cycle = soc->cycle;
    soc->cpu.mm_i_bti_req_mst = &soc->mm_i_bti_req_itf;
    soc->cpu.mm_i_bti_rsp_slv = &soc->mm_i_bti_rsp_itf;
    soc->cpu.mm_d_bti_req_mst = &soc->mm_d_bti_req_itf;
    soc->cpu.mm_d_bti_rsp_slv = &soc->mm_d_bti_rsp_itf;
    soc->cpu.peri_apb_req_mst = &soc->uart_apb_req_itf;
    soc->cpu.peri_apb_rsp_slv = &soc->uart_apb_rsp_itf;
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
        .aclint_mswi_base = ACLINT_MSWI_BASE,
        .aclint_mswi_size = ACLINT_MSWI_SIZE,
        .aclint_sswi_base = ACLINT_SSWI_BASE,
        .aclint_sswi_size = ACLINT_SSWI_SIZE,
        .plic_base = PLIC_BASE,
        .plic_size = PLIC_SIZE
    };
    DBG_VCD_MODULE_SCOPE("u_rv32g_cpu", rv32g_construct(&soc->cpu, &rv32g_conf));

    soc->flash.bti_req_slv = &soc->flash_bti_req_itf;
    soc->flash.bti_rsp_mst = &soc->flash_bti_rsp_itf;
    DBG_VCD_MODULE_SCOPE("u_flash", rom_construct(&soc->flash, FLASH_SIZE, NULL, 0, FLASH_BASE));

    soc->uart.apb_req_slv = &soc->uart_apb_req_itf;
    soc->uart.apb_rsp_mst = &soc->uart_apb_rsp_itf;
    soc->uart.uart_tx_mst = soc->uart_tx_mst;
    soc->uart.uart_rx_slv = soc->uart_rx_slv;
    DBG_VCD_MODULE_SCOPE("u_uart", uart_construct(&soc->uart, UART_BASE, UART_SIZE));

    soc->mm_d_bti_demux.host_bti_req_slv = &soc->mm_d_bti_req_itf;
    soc->mm_d_bti_demux.host_bti_rsp_mst = &soc->mm_d_bti_rsp_itf;
    soc->mm_d_bti_demux.gst_bti_req_msts[0] = &soc->ddr_d_bti_req_itf;
    soc->mm_d_bti_demux.gst_bti_rsp_slvs[0] = &soc->ddr_d_bti_rsp_itf;
    soc->mm_d_bti_demux.gst_bti_req_msts[1] = &soc->flash_bti_req_itf;
    soc->mm_d_bti_demux.gst_bti_rsp_slvs[1] = &soc->flash_bti_rsp_itf;
    const u32 mm_d_bti_gst_bases[] = { DDR_BASE, FLASH_BASE };
    const u32 mm_d_bti_gst_sizes[] = { DDR_SIZE, FLASH_SIZE };
    DBG_VCD_MODULE_SCOPE("u_mm_d_bti_demux", bti_demux_construct(&soc->mm_d_bti_demux, 2, mm_d_bti_gst_bases, mm_d_bti_gst_sizes));

    soc->ddr_bti_mux.host_bti_req_slvs[0] = &soc->mm_i_bti_req_itf;
    soc->ddr_bti_mux.host_bti_rsp_msts[0] = &soc->mm_i_bti_rsp_itf;
    soc->ddr_bti_mux.host_bti_req_slvs[1] = &soc->ddr_d_bti_req_itf;
    soc->ddr_bti_mux.host_bti_rsp_msts[1] = &soc->ddr_d_bti_rsp_itf;
    soc->ddr_bti_mux.gst_bti_req_mst = soc->ddr_bti_req_mst;
    soc->ddr_bti_mux.gst_bti_rsp_slv = soc->ddr_bti_rsp_slv;
    DBG_VCD_MODULE_SCOPE("u_ddr_bti_mux", bti_mux_construct(&soc->ddr_bti_mux, 2));
}

void soc_reset(soc_t *soc)
{
    rv32g_reset(&soc->cpu);
    rom_reset(&soc->flash);
    uart_reset(&soc->uart);
    bti_demux_reset(&soc->mm_d_bti_demux);
    bti_mux_reset(&soc->ddr_bti_mux);
}

void soc_clock(soc_t *soc)
{
    rv32g_clock(&soc->cpu);
    rom_clock(&soc->flash);
    uart_clock(&soc->uart);
    bti_demux_clock(&soc->mm_d_bti_demux);
    bti_mux_clock(&soc->ddr_bti_mux);

    itf_dbg_clock(&soc->mm_i_bti_req_itf);
    itf_dbg_clock(&soc->mm_i_bti_rsp_itf);
    itf_dbg_clock(&soc->mm_d_bti_req_itf);
    itf_dbg_clock(&soc->mm_d_bti_rsp_itf);
    itf_dbg_clock(&soc->ddr_d_bti_req_itf);
    itf_dbg_clock(&soc->ddr_d_bti_rsp_itf);
    itf_dbg_clock(&soc->flash_bti_req_itf);
    itf_dbg_clock(&soc->flash_bti_rsp_itf);
    itf_dbg_clock(&soc->uart_apb_req_itf);
    itf_dbg_clock(&soc->uart_apb_rsp_itf);
}

void soc_free(soc_t *soc)
{
    rv32g_free(&soc->cpu);
    rom_free(&soc->flash);
    uart_free(&soc->uart);
    bti_demux_free(&soc->mm_d_bti_demux);
    bti_mux_free(&soc->ddr_bti_mux);

    itf_free(&soc->mm_i_bti_req_itf);
    itf_free(&soc->mm_i_bti_rsp_itf);
    itf_free(&soc->mm_d_bti_req_itf);
    itf_free(&soc->mm_d_bti_rsp_itf);
    itf_free(&soc->ddr_d_bti_req_itf);
    itf_free(&soc->ddr_d_bti_rsp_itf);
    itf_free(&soc->flash_bti_req_itf);
    itf_free(&soc->flash_bti_rsp_itf);
    itf_free(&soc->uart_apb_req_itf);
    itf_free(&soc->uart_apb_rsp_itf);
}
