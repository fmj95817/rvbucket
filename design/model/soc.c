#include "soc.h"
#include "base/def.h"
#include "spec/core/core.h"
#include "spec/soc.h"
#include "dbg/vcd.h"

void soc_construct(soc_t *soc, const char *parent, const char *name)
{
    mod_construct(&soc->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    AXI4_IF_CONSTRUCT(soc, mm_i_, 1);
    AXI4_IF_CONSTRUCT(soc, mm_d_, 1);
    AXI4_IF_CONSTRUCT(soc, ddr_i_, 1);
    AXI4_IF_CONSTRUCT(soc, ddr_d_, 1);
    AXI4_IF_CONSTRUCT(soc, flash_i_, 1);
    AXI4_IF_CONSTRUCT(soc, flash_d_, 1);
    AXI4_IF_CONSTRUCT(soc, flash_, 1);
    APB_IF_CONSTRUCT(soc, peri_, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(soc, peri_uart_irq_itf, false, false);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(soc, peri_gpio_irq_itf, false, false);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(soc, peri_gtimer_irq_itf, false, false);

    soc->cpu.mod.cycle = soc->mod.cycle;
    AXI4_MST_CONNECT(&soc->cpu, mm_i_, soc, mm_i_);
    AXI4_MST_CONNECT(&soc->cpu, mm_d_, soc, mm_d_);
    APB_MST_CONNECT(&soc->cpu, peri_, soc, peri_);
    soc->cpu.ext_irq_ins[0] = &soc->peri_uart_irq_itf;
    soc->cpu.ext_irq_ins[1] = &soc->peri_gpio_irq_itf;
    soc->cpu.ext_irq_ins[2] = &soc->peri_gtimer_irq_itf;
    for (u32 i = 3; i < PLIC_MAX_IRQ_NUM; i++) {
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
    rv32g_construct(&soc->cpu, soc->mod.hier_name, "u_rv32g_cpu", &rv32g_conf);

    AXI4_SLV_CONNECT(&soc->flash, , soc, flash_);
    soc->flash.mod.cycle = soc->mod.cycle;
    rom_construct(&soc->flash, soc->mod.hier_name, "u_flash", ROM_MODE_AXI,
        FLASH_SIZE, NULL, 0, FLASH_BASE);

    soc->peri.mod.cycle = soc->mod.cycle;
    APB_SLV_CONNECT(&soc->peri, , soc, peri_);
    soc->peri.uart_tx_mst = soc->uart_tx_mst;
    soc->peri.uart_rx_slv = soc->uart_rx_slv;
    soc->peri.gpio_inout = soc->gpio_inout;
    soc->peri.uart_irq_out = &soc->peri_uart_irq_itf;
    soc->peri.gpio_irq_out = &soc->peri_gpio_irq_itf;
    soc->peri.gtimer_irq_out = &soc->peri_gtimer_irq_itf;
    peri_construct(&soc->peri, soc->mod.hier_name, "u_peri", PERI_BASE, PERI_SIZE);

    AXI4_SLV_CONNECT(&soc->mm_i_axi_demux, host_, soc, mm_i_);
    AXI4_MST_ARR_CONNECT(&soc->mm_i_axi_demux, gst_, 0, soc, ddr_i_);
    AXI4_MST_ARR_CONNECT(&soc->mm_i_axi_demux, gst_, 1, soc, flash_i_);
    const u32 mm_axi_gst_bases[] = { DDR_BASE, FLASH_BASE };
    const u32 mm_axi_gst_sizes[] = { DDR_SIZE, FLASH_SIZE };
    soc->mm_i_axi_demux.mod.cycle = soc->mod.cycle;
    axi_demux_construct(&soc->mm_i_axi_demux, soc->mod.hier_name,
        "u_mm_i_axi_demux", 2, mm_axi_gst_bases, mm_axi_gst_sizes);

    AXI4_SLV_CONNECT(&soc->mm_d_axi_demux, host_, soc, mm_d_);
    AXI4_MST_ARR_CONNECT(&soc->mm_d_axi_demux, gst_, 0, soc, ddr_d_);
    AXI4_MST_ARR_CONNECT(&soc->mm_d_axi_demux, gst_, 1, soc, flash_d_);
    soc->mm_d_axi_demux.mod.cycle = soc->mod.cycle;
    axi_demux_construct(&soc->mm_d_axi_demux, soc->mod.hier_name,
        "u_mm_d_axi_demux", 2, mm_axi_gst_bases, mm_axi_gst_sizes);

    AXI4_SLV_ARR_CONNECT(&soc->ddr_axi_mux, host_, 0, soc, ddr_i_);
    AXI4_SLV_ARR_CONNECT(&soc->ddr_axi_mux, host_, 1, soc, ddr_d_);
    AXI4_MST_IMPORT(&soc->ddr_axi_mux, gst_, soc, ddr_);
    soc->ddr_axi_mux.mod.cycle = soc->mod.cycle;
    axi_mux_construct(&soc->ddr_axi_mux, soc->mod.hier_name, "u_ddr_axi_mux", 2);

    AXI4_SLV_ARR_CONNECT(&soc->flash_axi_mux, host_, 0, soc, flash_i_);
    AXI4_SLV_ARR_CONNECT(&soc->flash_axi_mux, host_, 1, soc, flash_d_);
    AXI4_MST_CONNECT(&soc->flash_axi_mux, gst_, soc, flash_);
    soc->flash_axi_mux.mod.cycle = soc->mod.cycle;
    axi_mux_construct(&soc->flash_axi_mux, soc->mod.hier_name, "u_flash_axi_mux", 2);
}

void soc_reset(soc_t *soc)
{
    mod_reset(&soc->mod);
    rv32g_reset(&soc->cpu);
    rom_reset(&soc->flash);
    peri_reset(&soc->peri);
    axi_demux_reset(&soc->mm_i_axi_demux);
    axi_demux_reset(&soc->mm_d_axi_demux);
    axi_mux_reset(&soc->ddr_axi_mux);
    axi_mux_reset(&soc->flash_axi_mux);

    AXI4_IF_RESET(soc, mm_i_);
    AXI4_IF_RESET(soc, mm_d_);
    AXI4_IF_RESET(soc, ddr_i_);
    AXI4_IF_RESET(soc, ddr_d_);
    AXI4_IF_RESET(soc, flash_i_);
    AXI4_IF_RESET(soc, flash_d_);
    AXI4_IF_RESET(soc, flash_);
    APB_IF_RESET(soc, peri_);
    itf_reset(&soc->peri_uart_irq_itf);
    itf_reset(&soc->peri_gpio_irq_itf);
    itf_reset(&soc->peri_gtimer_irq_itf);
}

void soc_clock(soc_t *soc)
{
    mod_clock(&soc->mod);
    rv32g_clock(&soc->cpu);
    rom_clock(&soc->flash);
    peri_clock(&soc->peri);
    axi_demux_clock(&soc->mm_i_axi_demux);
    axi_demux_clock(&soc->mm_d_axi_demux);
    axi_mux_clock(&soc->ddr_axi_mux);
    axi_mux_clock(&soc->flash_axi_mux);

    AXI4_IF_DBG_CLOCK(soc, mm_i_);
    AXI4_IF_DBG_CLOCK(soc, mm_d_);
    AXI4_IF_DBG_CLOCK(soc, ddr_i_);
    AXI4_IF_DBG_CLOCK(soc, ddr_d_);
    AXI4_IF_DBG_CLOCK(soc, flash_i_);
    AXI4_IF_DBG_CLOCK(soc, flash_d_);
    AXI4_IF_DBG_CLOCK(soc, flash_);
    APB_IF_DBG_CLOCK(soc, peri_);
    itf_dbg_clock(&soc->peri_uart_irq_itf);
    itf_dbg_clock(&soc->peri_gpio_irq_itf);
    itf_dbg_clock(&soc->peri_gtimer_irq_itf);
}

void soc_free(soc_t *soc)
{
    mod_free(&soc->mod);
    rv32g_free(&soc->cpu);
    rom_free(&soc->flash);
    peri_free(&soc->peri);
    axi_demux_free(&soc->mm_i_axi_demux);
    axi_demux_free(&soc->mm_d_axi_demux);
    axi_mux_free(&soc->ddr_axi_mux);
    axi_mux_free(&soc->flash_axi_mux);

    AXI4_IF_FREE(soc, mm_i_);
    AXI4_IF_FREE(soc, mm_d_);
    AXI4_IF_FREE(soc, ddr_i_);
    AXI4_IF_FREE(soc, ddr_d_);
    AXI4_IF_FREE(soc, flash_i_);
    AXI4_IF_FREE(soc, flash_d_);
    AXI4_IF_FREE(soc, flash_);
    APB_IF_FREE(soc, peri_);
    itf_free(&soc->peri_uart_irq_itf);
    itf_free(&soc->peri_gpio_irq_itf);
    itf_free(&soc->peri_gtimer_irq_itf);
}
