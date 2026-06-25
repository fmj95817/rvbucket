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
    itf_t *ddr_bti_req_mst;
    itf_t *ddr_bti_rsp_slv;
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;
    itf_t *gpio_out;
    itf_t *ext_irq_ins[PLIC_MAX_IRQ_NUM];

    rv32g_t cpu;
    rom_t flash;
    peri_t peri;
    bti_demux_t mm_d_bti_demux;
    bti_mux_t ddr_bti_mux;

    itf_t mm_i_bti_req_itf;
    itf_t mm_i_bti_rsp_itf;
    itf_t mm_d_bti_req_itf;
    itf_t mm_d_bti_rsp_itf;
    itf_t ddr_d_bti_req_itf;
    itf_t ddr_d_bti_rsp_itf;
    itf_t flash_bti_req_itf;
    itf_t flash_bti_rsp_itf;
    itf_t peri_apb_req_itf;
    itf_t peri_apb_rsp_itf;
    itf_t peri_irq_sig_itf;
} soc_t;

extern void soc_construct(soc_t *soc, const char *name);
extern void soc_reset(soc_t *soc);
extern void soc_clock(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif