#include "io.h"

#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "spec/io/io.h"

void io_construct(io_t *io, const char *parent, const char *name,
    const io_conf_t *conf)
{
    mod_construct(&io->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf != NULL);
    DBG_CHECK(conf->size >= IO_SDSPI_OFFSET + IO_SDSPI_SIZE);

    io->base = conf->base;
    io->size = conf->size;
    u32 sdspi_base = conf->base + IO_SDSPI_OFFSET;

    APB_IF_CONSTRUCT(io, sdspi_, 1);

    APB_SLV_IMPORT(&io->apb_demux, host_, io, );
    APB_MST_ARR_CONNECT(&io->apb_demux, gst_, 0, io, sdspi_);
    const u32 apb_gst_bases[] = { sdspi_base };
    const u32 apb_gst_sizes[] = { IO_SDSPI_SIZE };
    io->apb_demux.mod.cycle = io->mod.cycle;
    apb_demux_construct(&io->apb_demux, io->mod.hier_name,
        "u_io_apb_demux", 1, apb_gst_bases, apb_gst_sizes);

    APB_SLV_CONNECT(&io->sdspi, , io, sdspi_);
    AXI4_MST_IMPORT(&io->sdspi, dma_, io, dma_);
    io->sdspi.irq_out = io->sdspi_irq_out;
    io->sdspi.cmd_mst = io->sdspi_cmd_mst;
    io->sdspi.data_slv = io->sdspi_data_slv;
    io->sdspi.mod.cycle = io->mod.cycle;
    sdspi_conf_t sdspi_conf = {
        .base_addr = sdspi_base,
        .size = IO_SDSPI_SIZE
    };
    sdspi_construct(&io->sdspi, io->mod.hier_name, "u_sdspi", &sdspi_conf);
}

void io_reset(io_t *io)
{
    mod_reset(&io->mod);
    APB_IF_RESET(io, sdspi_);
    apb_demux_reset(&io->apb_demux);
    sdspi_reset(&io->sdspi);
}

void io_clock(io_t *io)
{
    mod_clock(&io->mod);
    sdspi_clock(&io->sdspi);
    apb_demux_clock(&io->apb_demux);

    APB_IF_DBG_CLOCK(io, sdspi_);
}

void io_free(io_t *io)
{
    mod_free(&io->mod);
    sdspi_free(&io->sdspi);
    apb_demux_free(&io->apb_demux);

    APB_IF_FREE(io, sdspi_);
}
