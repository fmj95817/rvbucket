#ifndef PERI_H
#define PERI_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/axi4_if.h"
#include "itf/uart_if.h"
#include "itf/ext_irq_if.h"
#include "itf/gpio_if.h"
#include "bus/demux.h"
#include "peri/uart.h"
#include "peri/gpio.h"
#include "peri/gtimer.h"
#include "peri/pcm.h"
#include "io/ufshc.h"

typedef struct peri_conf {
    u32 base;
    u32 size;
    ufshc_conf_t ufshc;
} peri_conf_t;

typedef struct peri {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;
    itf_t *uart_irq_out;
    itf_t *gpio_inout;
    itf_t *gpio_irq_out;
    itf_t *gtimer_irq_out;
    itf_t *ufshc_irq_out;
    AXI4_MST_DECL(ufshc_);

    u32 base;
    u32 size;

    uart_t uart;
    gpio_t gpio;
    gtimer_t gtimer;
    pcm_t pcm;
    ufshc_t ufshc;
    apb_demux_t apb_demux;

    APB_IF_DECL(uart_);
    APB_IF_DECL(gpio_);
    APB_IF_DECL(gtimer_);
    APB_IF_DECL(pcm_);
    APB_IF_DECL(ufshc_);
} peri_t;

extern void peri_construct(peri_t *peri, const char *parent, const char *name,
    const peri_conf_t *conf);
extern void peri_reset(peri_t *peri);
extern void peri_clock(peri_t *peri);
extern void peri_free(peri_t *peri);

#endif
