#include "soc.h"
#include "base/def.h"
#include "spec/core/core.h"
#include "spec/core/hart.h"
#include "spec/soc.h"
#include "dbg/vcd.h"

void soc_construct(soc_t *soc, const char *parent, const char *name)
{
    mod_construct(&soc->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    AXI4_IF_CONSTRUCT(soc, mm_, 2);
    APB_IF_CONSTRUCT(soc, peri_, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(soc, peri_uart_irq_itf, false, false);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(soc, peri_gpio_irq_itf, false, false);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(soc, peri_gtimer_irq_itf, false, false);

    soc->cpu.mod.cycle = soc->mod.cycle;
    AXI4_MST_CONNECT(&soc->cpu, mm_, soc, mm_);
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
        .plic_size = PLIC_SIZE,
        .hart_ifu_ctrlq_depth = HART_IFU_CTRLQ_DEPTH,
        .hart_ifu_fch_ost_depth = HART_IFU_FCH_OST_DEPTH,
        .hart_hbi_stg_fifo_depth = HART_HBI_STG_FIFO_DEPTH,
        .hart_hbi_i_ost_depth = HART_HBI_I_OST_DEPTH,
        .hart_hbi_d_ost_depth = HART_HBI_D_OST_DEPTH,
        .hart_lsu_stg_fifo_depth = HART_LSU_STG_FIFO_DEPTH,
        .hart_lsu_ost_depth = HART_LSU_OST_DEPTH,
        .hart_mmu_i_stg_fifo_depth = HART_MMU_I_STG_FIFO_DEPTH,
        .hart_mmu_d_stg_fifo_depth = HART_MMU_D_STG_FIFO_DEPTH,
        .hart_mmu_ost_depth = HART_MMU_OST_DEPTH,
        .hart_l1i_stg_fifo_depth = HART_L1I_STG_FIFO_DEPTH,
        .hart_l1d_stg_fifo_depth = HART_L1D_STG_FIFO_DEPTH,
        .hart_l1_ost_depth = HART_L1_OST_DEPTH,
        .hart_l1d_bti_mux_stg_fifo_depth = HART_L1D_BTI_MUX_STG_FIFO_DEPTH,
        .hart_l1d_bti_mux_ost_depth = HART_L1D_BTI_MUX_OST_DEPTH,
        .cbi_bus_stg_fifo_depth = CORE_BUS_STG_FIFO_DEPTH,
        .cbi_bus_ost_depth = CORE_BUS_OST_DEPTH,
        .l2_stg_fifo_depth = CORE_L2_STG_FIFO_DEPTH,
        .l2_bypass_ost_depth = CORE_L2_BYPASS_OST_DEPTH
    };
    rv32g_construct(&soc->cpu, soc->mod.hier_name, "u_rv32g_cpu", &rv32g_conf);

    soc->peri.mod.cycle = soc->mod.cycle;
    APB_SLV_CONNECT(&soc->peri, , soc, peri_);
    soc->peri.uart_tx_mst = soc->uart_tx_mst;
    soc->peri.uart_rx_slv = soc->uart_rx_slv;
    soc->peri.gpio_inout = soc->gpio_inout;
    soc->peri.uart_irq_out = &soc->peri_uart_irq_itf;
    soc->peri.gpio_irq_out = &soc->peri_gpio_irq_itf;
    soc->peri.gtimer_irq_out = &soc->peri_gtimer_irq_itf;
    peri_construct(&soc->peri, soc->mod.hier_name, "u_peri", PERI_BASE, PERI_SIZE);

    AXI4_SLV_CONNECT(&soc->mm_axi_demux, host_, soc, mm_);
    AXI4_MST_ARR_IMPORT(&soc->mm_axi_demux, gst_, 0, soc, ddr_);
    AXI4_MST_ARR_IMPORT(&soc->mm_axi_demux, gst_, 1, soc, flash_);
    const u32 mm_axi_gst_bases[] = { DDR_BASE, FLASH_BASE };
    const u32 mm_axi_gst_sizes[] = { DDR_SIZE, FLASH_SIZE };
    soc->mm_axi_demux.mod.cycle = soc->mod.cycle;
    axi_demux_conf_t mm_axi_demux_conf = {
        .gst_num = 2,
        .gst_bases = mm_axi_gst_bases,
        .gst_sizes = mm_axi_gst_sizes,
        .stg_fifo_depth = SOC_BUS_STG_FIFO_DEPTH,
        .ost_depth = SOC_BUS_OST_DEPTH
    };
    axi_demux_construct(&soc->mm_axi_demux, soc->mod.hier_name,
        "u_mm_axi_demux", &mm_axi_demux_conf);
}

void soc_reset(soc_t *soc)
{
    mod_reset(&soc->mod);
    rv32g_reset(&soc->cpu);
    peri_reset(&soc->peri);
    axi_demux_reset(&soc->mm_axi_demux);

    AXI4_IF_RESET(soc, mm_);
    APB_IF_RESET(soc, peri_);
    itf_reset(&soc->peri_uart_irq_itf);
    itf_reset(&soc->peri_gpio_irq_itf);
    itf_reset(&soc->peri_gtimer_irq_itf);
}

void soc_clock(soc_t *soc)
{
    mod_clock(&soc->mod);
    rv32g_clock(&soc->cpu);
    peri_clock(&soc->peri);
    axi_demux_clock(&soc->mm_axi_demux);

    AXI4_IF_DBG_CLOCK(soc, mm_);
    APB_IF_DBG_CLOCK(soc, peri_);
    itf_dbg_clock(&soc->peri_uart_irq_itf);
    itf_dbg_clock(&soc->peri_gpio_irq_itf);
    itf_dbg_clock(&soc->peri_gtimer_irq_itf);
}

void soc_free(soc_t *soc)
{
    mod_free(&soc->mod);
    rv32g_free(&soc->cpu);
    peri_free(&soc->peri);
    axi_demux_free(&soc->mm_axi_demux);

    AXI4_IF_FREE(soc, mm_);
    APB_IF_FREE(soc, peri_);
    itf_free(&soc->peri_uart_irq_itf);
    itf_free(&soc->peri_gpio_irq_itf);
    itf_free(&soc->peri_gtimer_irq_itf);
}
