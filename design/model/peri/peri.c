#include "peri.h"
#include "spec/peri.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void peri_construct(peri_t *peri, const char *parent, const char *name, u32 base, u32 size)
{
    mod_construct(&peri->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    peri->base = base;
    peri->size = size;

    u32 uart_base  = base + PERI_UART_OFFSET;
    u32 gpio_base  = base + PERI_GPIO_OFFSET;
    u32 gtimer_base = base + PERI_GTIMER_OFFSET;
    u32 pcm_base = base + PERI_PCM_OFFSET;

    APB_IF_CONSTRUCT(peri, uart_, 1);
    APB_IF_CONSTRUCT(peri, gpio_, 1);
    APB_IF_CONSTRUCT(peri, gtimer_, 1);
    APB_IF_CONSTRUCT(peri, pcm_, 1);

    APB_SLV_IMPORT(&peri->apb_demux, host_, peri, );
    APB_MST_ARR_CONNECT(&peri->apb_demux, gst_, 0, peri, uart_);
    APB_MST_ARR_CONNECT(&peri->apb_demux, gst_, 1, peri, gpio_);
    APB_MST_ARR_CONNECT(&peri->apb_demux, gst_, 2, peri, gtimer_);
    APB_MST_ARR_CONNECT(&peri->apb_demux, gst_, 3, peri, pcm_);
    const u32 gst_bases[] = { uart_base, gpio_base, gtimer_base, pcm_base };
    const u32 gst_sizes[] = { PERI_UART_SIZE, PERI_GPIO_SIZE, PERI_GTIMER_SIZE,
        PERI_PCM_SIZE };
    peri->apb_demux.mod.cycle = peri->mod.cycle;
    apb_demux_construct(&peri->apb_demux, peri->mod.hier_name,
        "u_peri_apb_demux", 4, gst_bases, gst_sizes);

    APB_SLV_CONNECT(&peri->uart, , peri, uart_);
    peri->uart.uart_tx_mst = peri->uart_tx_mst;
    peri->uart.uart_rx_slv = peri->uart_rx_slv;
    peri->uart.irq_out = peri->uart_irq_out;
    peri->uart.mod.cycle = peri->mod.cycle;
    uart_construct(&peri->uart, peri->mod.hier_name, "u_uart", uart_base,
        PERI_UART_SIZE);

    APB_SLV_CONNECT(&peri->gpio, , peri, gpio_);
    peri->gpio.inout_sig = peri->gpio_inout;
    peri->gpio.irq_out = peri->gpio_irq_out;
    peri->gpio.mod.cycle = peri->mod.cycle;
    gpio_construct(&peri->gpio, peri->mod.hier_name, "u_gpio", gpio_base,
        PERI_GPIO_SIZE);

    APB_SLV_CONNECT(&peri->gtimer, , peri, gtimer_);
    peri->gtimer.irq_out = peri->gtimer_irq_out;
    peri->gtimer.mod.cycle = peri->mod.cycle;
    gtimer_construct(&peri->gtimer, peri->mod.hier_name, "u_gtimer", gtimer_base,
        PERI_GTIMER_SIZE);

    APB_SLV_CONNECT(&peri->pcm, , peri, pcm_);
    peri->pcm.mod.cycle = peri->mod.cycle;
    pcm_construct(&peri->pcm, peri->mod.hier_name, "u_pcm", pcm_base,
        PERI_PCM_SIZE);
}

void peri_reset(peri_t *peri)
{
    mod_reset(&peri->mod);
    uart_reset(&peri->uart);
    gpio_reset(&peri->gpio);
    gtimer_reset(&peri->gtimer);
    pcm_reset(&peri->pcm);
    apb_demux_reset(&peri->apb_demux);

    APB_IF_RESET(peri, uart_);
    APB_IF_RESET(peri, gpio_);
    APB_IF_RESET(peri, gtimer_);
    APB_IF_RESET(peri, pcm_);
}

void peri_clock(peri_t *peri)
{
    mod_clock(&peri->mod);
    uart_clock(&peri->uart);
    gpio_clock(&peri->gpio);
    gtimer_clock(&peri->gtimer);
    pcm_clock(&peri->pcm);
    apb_demux_clock(&peri->apb_demux);

    APB_IF_DBG_CLOCK(peri, uart_);
    APB_IF_DBG_CLOCK(peri, gpio_);
    APB_IF_DBG_CLOCK(peri, gtimer_);
    APB_IF_DBG_CLOCK(peri, pcm_);
}

void peri_free(peri_t *peri)
{
    mod_free(&peri->mod);
    uart_free(&peri->uart);
    gpio_free(&peri->gpio);
    gtimer_free(&peri->gtimer);
    pcm_free(&peri->pcm);
    apb_demux_free(&peri->apb_demux);

    APB_IF_FREE(peri, uart_);
    APB_IF_FREE(peri, gpio_);
    APB_IF_FREE(peri, gtimer_);
    APB_IF_FREE(peri, pcm_);
}
