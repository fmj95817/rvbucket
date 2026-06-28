#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "soc.h"
#include "itf/bti_if.h"
#include "itf/axi4_if.h"
#include "itf/uart_if.h"
#include "itf/gpio_if.h"
#include "mem/ram.h"
#include "dbg/chk.h"
#include "dbg/log.h"
#include "dbg/env.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"
#include "spec/soc.h"
#include "ui_def.h"

#define SIM_END_CHAR 0x10
#define BIN_TYPE_LINUX 1u
#define LINUX_BIN_HEADER_SIZE 40u
#define UART_IN_POLL_INTERVAL 256u

typedef struct program {
    u32 size;
    u8 *code;
} program_t;

typedef struct sim_top {
    u64 *cycle;
    itf_t uart_rx_itf;
    itf_t uart_tx_itf;
    AXI4_IF_DECL(ddr_);
    itf_t gpio_sig_itf;
    gpio_if_t *gpio_o;
    ram_t ddr;
    soc_t soc;
    bool end_sim;
    sim_ui_t *ui;
} sim_top_t;

static void sim_top_gpio_cb(void *args)
{
    sim_top_t *s = (sim_top_t *)args;
    u32 val = s->gpio_o->val;
    DBG_LOG(LOG_INFO, "gpio: output = 0x%08x\n", val);
    if (s->ui) s->ui->gpio_change(s->ui, val);
}

static program_t read_program(const char *path)
{
    program_t program;
    FILE *fp = fopen(path, "rb");
    fseek(fp, 0L, SEEK_END);
    program.size = (u32)ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    program.code = malloc(program.size);
    (void)!fread(program.code, 1, program.size, fp);
    fclose(fp);
    return program;
}

static u32 read_u32_le(const u8 *data)
{
    return ((u32)data[0]) | ((u32)data[1] << 8u) |
           ((u32)data[2] << 16u) | ((u32)data[3] << 24u);
}

static void write_u32_le(u8 *data, u32 val)
{
    data[0] = val & 0xffu; data[1] = (val >> 8u) & 0xffu;
    data[2] = (val >> 16u) & 0xffu; data[3] = (val >> 24u) & 0xffu;
}

static u32 align4(u32 val) { return (val + 3u) & ~3u; }

static void sim_top_preload_linux(sim_top_t *sim_top, program_t *program)
{
    if (program->size < LINUX_BIN_HEADER_SIZE) return;
    if (read_u32_le(program->code) != BIN_TYPE_LINUX) return;

    u32 itcm_size = read_u32_le(program->code + 4);
    u32 dtcm_size = read_u32_le(program->code + 8);
    u32 kernel_size = read_u32_le(program->code + 12);
    u32 initrd_size = read_u32_le(program->code + 16);
    u32 dtb_size = read_u32_le(program->code + 20);
    u32 kernel_load = read_u32_le(program->code + 24);
    u32 initrd_load = read_u32_le(program->code + 28);
    u32 dtb_load = read_u32_le(program->code + 32);

    u32 off = LINUX_BIN_HEADER_SIZE + align4(itcm_size) + align4(dtcm_size);
    DBG_CHECK(off <= program->size);
    DBG_CHECK(off + align4(kernel_size) <= program->size);
    ram_load(&sim_top->ddr, program->code + off, kernel_load - DDR_BASE, kernel_size);

    off += align4(kernel_size);
    DBG_CHECK(off + align4(initrd_size) <= program->size);
    ram_load(&sim_top->ddr, program->code + off, initrd_load - DDR_BASE, initrd_size);

    off += align4(initrd_size);
    DBG_CHECK(off + align4(dtb_size) <= program->size);
    ram_load(&sim_top->ddr, program->code + off, dtb_load - DDR_BASE, dtb_size);

    write_u32_le(program->code + 12, 0);
    write_u32_le(program->code + 16, 0);
    write_u32_le(program->code + 20, 0);

    fprintf(stderr, "sim_top: preloaded linux payloads to DDR: kernel %u bytes @ 0x%08x, initrd %u bytes @ 0x%08x, dtb %u bytes @ 0x%08x\n",
        kernel_size, kernel_load, initrd_size, initrd_load, dtb_size, dtb_load);
}

static void soc_burn_program(sim_top_t *sim_top, const char *path, bool fast_load_linux)
{
    program_t program = read_program(path);
    if (fast_load_linux) sim_top_preload_linux(sim_top, &program);
    rom_burn(&sim_top->soc.flash, program.code, 0, program.size);
    free(program.code);
}

static void sim_top_construct(sim_top_t *sim_top, const char *name,
    const char *prog_path, bool fast_load_linux)
{
    DBG_VCD_MODULE_SCOPE(name);
    sim_top->cycle = dbg_pcm_reg_perf_cnt("cycles");

    UART_IF_CONSTRUCT(sim_top, uart_rx_itf, 1);
    UART_IF_CONSTRUCT(sim_top, uart_tx_itf, 1);
    AXI4_IF_CONSTRUCT(sim_top, ddr_);
    GPIO_SIGNAL_IF_CONSTRUCT(sim_top, gpio_sig_itf, false, false);

    sim_top->gpio_o = itf_signal_get_src_and_chk(&sim_top->gpio_sig_itf);
    itf_signal_set_wcb(&sim_top->gpio_sig_itf, &sim_top_gpio_cb, sim_top);

    sim_top->soc.cycle = sim_top->cycle;
    AXI4_MST_CONNECT(&sim_top->soc, ddr_, sim_top, ddr_);
    sim_top->soc.uart_tx_mst = &sim_top->uart_rx_itf;
    sim_top->soc.uart_rx_slv = &sim_top->uart_tx_itf;
    sim_top->soc.gpio_out = &sim_top->gpio_sig_itf;
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        sim_top->soc.ext_irq_ins[i] = NULL;
    }
    soc_construct(&sim_top->soc, "u_soc");

    if (dbg_get_bool_env("LOG_UI")) {
        sim_top->ui = ui_web_create();
    } else {
        sim_top->ui = ui_term_create();
    }

    AXI4_SLV_CONNECT(&sim_top->ddr, , sim_top, ddr_);
    ram_construct(&sim_top->ddr, "u_ddr", 1, RAM_MODE_AXI, DDR_SIZE, DDR_BASE);

    dbg_vcd_set_clk(sim_top->cycle);
    dbg_vcd_add_sig("cycle", DBG_SIG_TYPE_REG, 64, sim_top->cycle);

    soc_burn_program(sim_top, prog_path, fast_load_linux);
}

static void sim_top_reset(sim_top_t *sim_top)
{
    sim_top->end_sim = false;
    if (sim_top->ui && sim_top->ui->reset) {
        sim_top->ui->reset(sim_top->ui);
    }
    soc_reset(&sim_top->soc);
    dbg_vcd_reset();

    itf_reset(&sim_top->uart_rx_itf);
    itf_reset(&sim_top->uart_tx_itf);
    AXI4_IF_RESET(sim_top, ddr_);
    itf_reset(&sim_top->gpio_sig_itf);
}

static void sim_top_clock(sim_top_t *sim_top)
{
    /* poll UART input */
    if (((*sim_top->cycle) % UART_IN_POLL_INTERVAL) == 0u) {
        u8 ch;
        while (!itf_fifo_full(&sim_top->uart_tx_itf) &&
               sim_top->ui->uart_in(sim_top->ui, &ch)) {
            uart_if_t pkt;
            pkt.data = ch;
            itf_write(&sim_top->uart_tx_itf, &pkt);
        }
    }

    soc_clock(&sim_top->soc);
    ram_clock(&sim_top->ddr);

    itf_dbg_clock(&sim_top->uart_rx_itf);
    itf_dbg_clock(&sim_top->uart_tx_itf);
    AXI4_IF_DBG_CLOCK(sim_top, ddr_);
    itf_dbg_clock(&sim_top->gpio_sig_itf);

    (*sim_top->cycle)++;
    dbg_vcd_clock();

    /* UART output */
    if (!itf_fifo_empty(&sim_top->uart_rx_itf)) {
        uart_if_t pkt;
        itf_read(&sim_top->uart_rx_itf, &pkt);

        i32 c;
        c.u = pkt.data;
        if (c.s != SIM_END_CHAR) {
            sim_top->ui->uart_out(sim_top->ui, (u8)c.s);
        } else {
            sim_top->end_sim = true;
        }
    }
}

static void sim_top_free(sim_top_t *sim_top)
{
    if (sim_top->ui) {
        sim_top->ui->cleanup(sim_top->ui);
    }
    soc_free(&sim_top->soc);
    ram_free(&sim_top->ddr);

    itf_free(&sim_top->uart_rx_itf);
    itf_free(&sim_top->uart_tx_itf);
    AXI4_IF_FREE(sim_top, ddr_);
    itf_free(&sim_top->gpio_sig_itf);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s [--fast-load-linux] <program.bin>\n", argv[0]);
        return 0;
    }
    bool fast_load_linux = false;
    const char *prog_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fast-load-linux") == 0) {
            fast_load_linux = true;
        } else if (!prog_path) {
            prog_path = argv[i];
        } else {
            fprintf(stderr, "usage: %s [--fast-load-linux] <program.bin>\n", argv[0]);
            return 1;
        }
    }
    if (!prog_path) {
        fprintf(stderr, "usage: %s [--fast-load-linux] <program.bin>\n", argv[0]);
        return 1;
    }

    sim_top_t sim_top;
    sim_top_construct(&sim_top, "sim_top", prog_path, fast_load_linux);
    sim_top_reset(&sim_top);

    while (!sim_top.end_sim) {
        sim_top_clock(&sim_top);
    }

    sim_top_reset(&sim_top);
    while (!sim_top.end_sim) {
        sim_top_clock(&sim_top);
    }

    sim_top_free(&sim_top);
    return 0;
}
