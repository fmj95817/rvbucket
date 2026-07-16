#ifndef IO_IO_H
#define IO_IO_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/axi4_if.h"
#include "bus/demux.h"
#include "io/sdspi.h"

typedef struct io_conf {
    u32 base;
    u32 size;
} io_conf_t;

typedef struct io {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    AXI4_MST_DECL(dma_);
    itf_t *sdspi_irq_out;
    itf_t *sdspi_cmd_mst;
    itf_t *sdspi_data_slv;

    u32 base;
    u32 size;

    sdspi_t sdspi;
    apb_demux_t apb_demux;

    APB_IF_DECL(sdspi_);
} io_t;

extern void io_construct(io_t *io, const char *parent, const char *name,
    const io_conf_t *conf);
extern void io_reset(io_t *io);
extern void io_clock(io_t *io);
extern void io_free(io_t *io);

#endif
