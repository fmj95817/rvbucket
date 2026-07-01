#ifndef PERI_H
#define PERI_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/uart_if.h"
#include "itf/ext_irq_if.h"
#include "itf/gpio_if.h"
#include "bus/demux.h"
#include "peri/uart.h"
#include "peri/gpio.h"
#include "peri/gtimer.h"

typedef struct peri {
    const u64 *cycle;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;
    itf_t *uart_irq_out;
    itf_t *gpio_inout;
    itf_t *gpio_irq_out;
    itf_t *gtimer_irq_out;

    u32 base;
    u32 size;

    uart_t uart;
    gpio_t gpio;
    gtimer_t gtimer;
    apb_demux_t apb_demux;

    APB_IF_DECL(uart_);
    APB_IF_DECL(gpio_);
    APB_IF_DECL(gtimer_);
} peri_t;

extern void peri_construct(peri_t *peri, const char *name, u32 base, u32 size);
extern void peri_reset(peri_t *peri);
extern void peri_clock(peri_t *peri);
extern void peri_free(peri_t *peri);

#endif
