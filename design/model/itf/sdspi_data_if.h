#ifndef SDSPI_DATA_IF_H
#define SDSPI_DATA_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define SDSPI_DATA_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    SDSPI_DATA_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define SDSPI_DATA_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    SDSPI_DATA_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define SDSPI_DATA_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(sdspi_data_if_t), \
        .pkt2str = &sdspi_data_if_to_str, \
        .reg_vcd = &sdspi_data_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define SDSPI_DATA_IF_CONSTRUCT(module, itf, depth) \
    SDSPI_DATA_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define SDSPI_DATA_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    SDSPI_DATA_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define SDSPI_DATA_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(sdspi_data_if_t), \
        .pkt2str = &sdspi_data_if_to_str, \
        .reg_vcd = &sdspi_data_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum sdspi_data_kind {
    SDSPI_DATA_KIND_STATUS = 0,
    SDSPI_DATA_KIND_RESPONSE = 1,
    SDSPI_DATA_KIND_READ_DATA = 2,
    SDSPI_DATA_KIND_WRITE_DONE = 3
} sdspi_data_kind_t;

typedef struct sdspi_data_if {
    sdspi_data_kind_t kind;
    bool card_present;
    bool read_only;
    bool card_idle;
    u32 resp0;
    u32 resp1;
    u32 data;
    bool last;
    bool error;
    u32 cmd_error;
    u32 data_error;
} sdspi_data_if_t;

static inline void sdspi_data_if_to_str(const void *pkt, char *str)
{
    const sdspi_data_if_t *sdspi_data = (const sdspi_data_if_t *)pkt;
    sprintf(str, "%01x %01x %01x %01x %08x %08x %08x %01x %01x %08x %08x\n", (u32)sdspi_data->kind, sdspi_data->card_present, sdspi_data->read_only, sdspi_data->card_idle, sdspi_data->resp0, sdspi_data->resp1, sdspi_data->data, sdspi_data->last, sdspi_data->error, sdspi_data->cmd_error, sdspi_data->data_error);
}

static inline void sdspi_data_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const sdspi_data_if_t *sdspi_data = (const sdspi_data_if_t *)pkt;
    dbg_vcd_add_sig("kind", type, 2, &sdspi_data->kind);
    dbg_vcd_add_sig("card_present", type, 1, &sdspi_data->card_present);
    dbg_vcd_add_sig("read_only", type, 1, &sdspi_data->read_only);
    dbg_vcd_add_sig("card_idle", type, 1, &sdspi_data->card_idle);
    dbg_vcd_add_sig("resp0", type, 32, &sdspi_data->resp0);
    dbg_vcd_add_sig("resp1", type, 32, &sdspi_data->resp1);
    dbg_vcd_add_sig("data", type, 32, &sdspi_data->data);
    dbg_vcd_add_sig("last", type, 1, &sdspi_data->last);
    dbg_vcd_add_sig("error", type, 1, &sdspi_data->error);
    dbg_vcd_add_sig("cmd_error", type, 32, &sdspi_data->cmd_error);
    dbg_vcd_add_sig("data_error", type, 32, &sdspi_data->data_error);
}

#endif
