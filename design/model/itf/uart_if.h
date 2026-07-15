#ifndef UART_IF_H
#define UART_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define UART_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    UART_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define UART_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    UART_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define UART_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(uart_if_t), \
        .pkt2str = &uart_if_to_str, \
        .reg_vcd = &uart_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define UART_IF_CONSTRUCT(module, itf, depth) \
    UART_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define UART_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    UART_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define UART_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(uart_if_t), \
        .pkt2str = &uart_if_to_str, \
        .reg_vcd = &uart_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct uart_if {
    u32 data;
} uart_if_t;

static inline void uart_if_to_str(const void *pkt, char *str)
{
    const uart_if_t *uart = (const uart_if_t *)pkt;
    sprintf(str, "%08x\n", uart->data);
}

static inline void uart_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const uart_if_t *uart = (const uart_if_t *)pkt;
    dbg_vcd_add_sig("data", type, 32, &uart->data);
}

#endif
