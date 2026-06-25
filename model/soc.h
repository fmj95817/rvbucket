#ifndef SOC_H
#define SOC_H

#include "base/types.h"
#include "core/rv32g.h"
#include "mem/ram.h"
#include "mem/rom.h"
#include "peri/peri.h"
#include "bus/mux.h"
#include "bus/demux.h"

typedef struct soc {
    const u64 *cycle;
    itf_t *ddr_axi4_aw_mst;
    itf_t *ddr_axi4_w_mst;
    itf_t *ddr_axi4_b_slv;
    itf_t *ddr_axi4_ar_mst;
    itf_t *ddr_axi4_r_slv;
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;
    itf_t *gpio_out;
    itf_t *ext_irq_ins[PLIC_MAX_IRQ_NUM];

    rv32g_t cpu;
    rom_t flash;
    peri_t peri;
    axi_demux_t mm_i_axi_demux;
    axi_demux_t mm_d_axi_demux;
    axi_mux_t ddr_axi_mux;
    axi_mux_t flash_axi_mux;

    itf_t mm_i_axi4_aw_itf;
    itf_t mm_i_axi4_w_itf;
    itf_t mm_i_axi4_b_itf;
    itf_t mm_i_axi4_ar_itf;
    itf_t mm_i_axi4_r_itf;
    itf_t mm_d_axi4_aw_itf;
    itf_t mm_d_axi4_w_itf;
    itf_t mm_d_axi4_b_itf;
    itf_t mm_d_axi4_ar_itf;
    itf_t mm_d_axi4_r_itf;
    itf_t ddr_i_axi4_aw_itf;
    itf_t ddr_i_axi4_w_itf;
    itf_t ddr_i_axi4_b_itf;
    itf_t ddr_i_axi4_ar_itf;
    itf_t ddr_i_axi4_r_itf;
    itf_t ddr_d_axi4_aw_itf;
    itf_t ddr_d_axi4_w_itf;
    itf_t ddr_d_axi4_b_itf;
    itf_t ddr_d_axi4_ar_itf;
    itf_t ddr_d_axi4_r_itf;
    itf_t flash_i_axi4_aw_itf;
    itf_t flash_i_axi4_w_itf;
    itf_t flash_i_axi4_b_itf;
    itf_t flash_i_axi4_ar_itf;
    itf_t flash_i_axi4_r_itf;
    itf_t flash_d_axi4_aw_itf;
    itf_t flash_d_axi4_w_itf;
    itf_t flash_d_axi4_b_itf;
    itf_t flash_d_axi4_ar_itf;
    itf_t flash_d_axi4_r_itf;
    itf_t flash_axi4_aw_itf;
    itf_t flash_axi4_w_itf;
    itf_t flash_axi4_b_itf;
    itf_t flash_axi4_ar_itf;
    itf_t flash_axi4_r_itf;
    itf_t peri_apb_req_itf;
    itf_t peri_apb_rsp_itf;
    itf_t peri_irq_sig_itf;
} soc_t;

extern void soc_construct(soc_t *soc, const char *name);
extern void soc_reset(soc_t *soc);
extern void soc_clock(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif
