#include "peri.h"
#include "spec/peri.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void peri_construct(peri_t *peri, const char *name, u32 base, u32 size)
{
    DBG_VCD_MODULE_SCOPE(name);

    peri->base = base;
    peri->size = size;

    u32 uart_base  = base + PERI_UART_OFFSET;
    u32 gpio_base  = base + PERI_GPIO_OFFSET;
    u32 timer_base = base + PERI_TIMER_OFFSET;

    APB_REQ_IF_CONSTRUCT(peri, uart_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(peri, uart_apb_rsp_itf, 1);
    APB_REQ_IF_CONSTRUCT(peri, gpio_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(peri, gpio_apb_rsp_itf, 1);
    APB_REQ_IF_CONSTRUCT(peri, timer_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(peri, timer_apb_rsp_itf, 1);

    peri->apb_demux.host_apb_req_slv = peri->apb_req_slv;
    peri->apb_demux.host_apb_rsp_mst = peri->apb_rsp_mst;
    peri->apb_demux.gst_apb_req_msts[0] = &peri->uart_apb_req_itf;
    peri->apb_demux.gst_apb_rsp_slvs[0] = &peri->uart_apb_rsp_itf;
    peri->apb_demux.gst_apb_req_msts[1] = &peri->gpio_apb_req_itf;
    peri->apb_demux.gst_apb_rsp_slvs[1] = &peri->gpio_apb_rsp_itf;
    peri->apb_demux.gst_apb_req_msts[2] = &peri->timer_apb_req_itf;
    peri->apb_demux.gst_apb_rsp_slvs[2] = &peri->timer_apb_rsp_itf;
    const u32 gst_bases[] = { uart_base, gpio_base, timer_base };
    const u32 gst_sizes[] = { PERI_UART_SIZE, PERI_GPIO_SIZE, PERI_TIMER_SIZE };
    apb_demux_construct(&peri->apb_demux, "u_peri_apb_demux", 3, gst_bases, gst_sizes);

    peri->uart.apb_req_slv = &peri->uart_apb_req_itf;
    peri->uart.apb_rsp_mst = &peri->uart_apb_rsp_itf;
    peri->uart.uart_tx_mst = peri->uart_tx_mst;
    peri->uart.uart_rx_slv = peri->uart_rx_slv;
    peri->uart.irq_out = peri->uart_irq_out;
    uart_construct(&peri->uart, "u_uart", uart_base, PERI_UART_SIZE);

    peri->gpio.apb_req_slv = &peri->gpio_apb_req_itf;
    peri->gpio.apb_rsp_mst = &peri->gpio_apb_rsp_itf;
    peri->gpio.inout_sig = peri->gpio_inout;
    peri->gpio.irq_out = peri->gpio_irq_out;
    gpio_construct(&peri->gpio, "u_gpio", gpio_base, PERI_GPIO_SIZE);

    peri->timer.apb_req_slv = &peri->timer_apb_req_itf;
    peri->timer.apb_rsp_mst = &peri->timer_apb_rsp_itf;
    peri->timer.irq_out = peri->timer_irq_out;
    timer_construct(&peri->timer, "u_timer", timer_base, PERI_TIMER_SIZE);
}

void peri_reset(peri_t *peri)
{
    uart_reset(&peri->uart);
    gpio_reset(&peri->gpio);
    timer_reset(&peri->timer);
    apb_demux_reset(&peri->apb_demux);

    itf_reset(&peri->uart_apb_req_itf);
    itf_reset(&peri->uart_apb_rsp_itf);
    itf_reset(&peri->gpio_apb_req_itf);
    itf_reset(&peri->gpio_apb_rsp_itf);
    itf_reset(&peri->timer_apb_req_itf);
    itf_reset(&peri->timer_apb_rsp_itf);
}

void peri_clock(peri_t *peri)
{
    uart_clock(&peri->uart);
    gpio_clock(&peri->gpio);
    timer_clock(&peri->timer);
    apb_demux_clock(&peri->apb_demux);

    itf_dbg_clock(&peri->uart_apb_req_itf);
    itf_dbg_clock(&peri->uart_apb_rsp_itf);
    itf_dbg_clock(&peri->gpio_apb_req_itf);
    itf_dbg_clock(&peri->gpio_apb_rsp_itf);
    itf_dbg_clock(&peri->timer_apb_req_itf);
    itf_dbg_clock(&peri->timer_apb_rsp_itf);
}

void peri_free(peri_t *peri)
{
    uart_free(&peri->uart);
    gpio_free(&peri->gpio);
    timer_free(&peri->timer);
    apb_demux_free(&peri->apb_demux);

    itf_free(&peri->uart_apb_req_itf);
    itf_free(&peri->uart_apb_rsp_itf);
    itf_free(&peri->gpio_apb_req_itf);
    itf_free(&peri->gpio_apb_rsp_itf);
    itf_free(&peri->timer_apb_req_itf);
    itf_free(&peri->timer_apb_rsp_itf);
}
