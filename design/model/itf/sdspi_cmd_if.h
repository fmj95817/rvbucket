#ifndef SDSPI_CMD_IF_H
#define SDSPI_CMD_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define SDSPI_CMD_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    SDSPI_CMD_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define SDSPI_CMD_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    SDSPI_CMD_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define SDSPI_CMD_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(sdspi_cmd_if_t), \
        .pkt2str = &sdspi_cmd_if_to_str, \
        .reg_vcd = &sdspi_cmd_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define SDSPI_CMD_IF_CONSTRUCT(module, itf, depth) \
    SDSPI_CMD_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define SDSPI_CMD_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    SDSPI_CMD_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define SDSPI_CMD_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(sdspi_cmd_if_t), \
        .pkt2str = &sdspi_cmd_if_to_str, \
        .reg_vcd = &sdspi_cmd_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum sdspi_cmd_kind {
    SDSPI_CMD_KIND_COMMAND = 0,
    SDSPI_CMD_KIND_WRITE_DATA = 1,
    SDSPI_CMD_KIND_RESET = 2,
    SDSPI_CMD_KIND_CONFIG = 3
} sdspi_cmd_kind_t;

typedef struct sdspi_cmd_if {
    sdspi_cmd_kind_t kind;
    bool enable;
    u32 clock_div;
    u8 opcode; // 6-bit
    u32 arg;
    u8 rsp_type; // 3-bit
    bool data_present;
    bool write;
    u32 block_size;
    u32 block_count;
    u32 data_len;
    u32 data;
    bool last;
} sdspi_cmd_if_t;

static inline void sdspi_cmd_if_to_str(const void *pkt, char *str)
{
    const sdspi_cmd_if_t *sdspi_cmd = (const sdspi_cmd_if_t *)pkt;
    sprintf(str, "%01x %01x %08x %02x %08x %01x %01x %01x %08x %08x %08x %08x %01x\n", (u32)sdspi_cmd->kind, sdspi_cmd->enable, sdspi_cmd->clock_div, sdspi_cmd->opcode, sdspi_cmd->arg, sdspi_cmd->rsp_type, sdspi_cmd->data_present, sdspi_cmd->write, sdspi_cmd->block_size, sdspi_cmd->block_count, sdspi_cmd->data_len, sdspi_cmd->data, sdspi_cmd->last);
}

static inline void sdspi_cmd_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const sdspi_cmd_if_t *sdspi_cmd = (const sdspi_cmd_if_t *)pkt;
    dbg_vcd_add_sig("kind", type, 2, &sdspi_cmd->kind);
    dbg_vcd_add_sig("enable", type, 1, &sdspi_cmd->enable);
    dbg_vcd_add_sig("clock_div", type, 32, &sdspi_cmd->clock_div);
    dbg_vcd_add_sig("opcode", type, 6, &sdspi_cmd->opcode);
    dbg_vcd_add_sig("arg", type, 32, &sdspi_cmd->arg);
    dbg_vcd_add_sig("rsp_type", type, 3, &sdspi_cmd->rsp_type);
    dbg_vcd_add_sig("data_present", type, 1, &sdspi_cmd->data_present);
    dbg_vcd_add_sig("write", type, 1, &sdspi_cmd->write);
    dbg_vcd_add_sig("block_size", type, 32, &sdspi_cmd->block_size);
    dbg_vcd_add_sig("block_count", type, 32, &sdspi_cmd->block_count);
    dbg_vcd_add_sig("data_len", type, 32, &sdspi_cmd->data_len);
    dbg_vcd_add_sig("data", type, 32, &sdspi_cmd->data);
    dbg_vcd_add_sig("last", type, 1, &sdspi_cmd->last);
}

#endif
