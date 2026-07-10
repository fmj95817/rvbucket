#ifndef SOC_H
#define SOC_H

#include "base/types.h"
#include "base/mod.h"
#include "core/rv32g.h"
#include "itf/axi4_if.h"
#include "peri/peri.h"
#include "bus/demux.h"

typedef struct soc {
    mod_t mod;
    AXI4_MST_DECL(ddr_);
    AXI4_MST_DECL(flash_);
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;
    itf_t *gpio_inout;
    itf_t *ext_irq_ins[PLIC_MAX_IRQ_NUM];

    rv32g_t cpu;
    peri_t peri;
    axi_demux_t mm_axi_demux;

    AXI4_IF_DECL(mm_);
    APB_IF_DECL(peri_);
    itf_t peri_uart_irq_itf;
    itf_t peri_gpio_irq_itf;
    itf_t peri_gtimer_irq_itf;
} soc_t;

extern void soc_construct(soc_t *soc, const char *parent, const char *name,
    bool perf_sim);
extern void soc_reset(soc_t *soc);
extern void soc_clock(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif
