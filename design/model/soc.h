#ifndef SOC_H
#define SOC_H

#include "base/types.h"
#include "base/mod.h"
#include "core/rv32g.h"
#include "itf/axi4_if.h"
#include "peri/peri.h"
#include "io/io.h"
#include "bus/demux.h"
#include "bus/mux.h"

typedef struct soc_conf {
    bool perf_sim;
    bool smp_opt;
} soc_conf_t;

typedef struct soc {
    mod_t mod;
    AXI4_MST_DECL(ddr_);
    AXI4_MST_DECL(flash_);
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;
    itf_t *gpio_inout;
    itf_t *sdspi_cmd_mst;
    itf_t *sdspi_data_slv;
    itf_t *ext_irq_ins[PLIC_MAX_IRQ_NUM];

    rv32g_t cpu;
    peri_t peri;
    io_t io;
    axi_demux_t mm_axi_demux;
    axi_mux_t ddr_axi_mux;
    
    AXI4_IF_DECL(mm_);
    AXI4_IF_DECL(cpu_ddr_);
    AXI4_IF_DECL(io_dma_);
    APB_IF_DECL(peri_);
    APB_IF_DECL(io_);
    itf_t peri_uart_irq_itf;
    itf_t peri_gpio_irq_itf;
    itf_t peri_gtimer_irq_itf;
    itf_t io_sdspi_irq_itf;

    u64 *perf_cycle;
} soc_t;

extern void soc_construct(soc_t *soc, const char *parent, const char *name,
    const soc_conf_t *conf);
extern void soc_reset(soc_t *soc);
extern void soc_clock(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif
