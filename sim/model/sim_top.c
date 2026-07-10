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
#include "mem/rom.h"
#include "base/smp_opt.h"
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
#define UART_IN_POLL_INTERVAL 500u
#define GPIO_IN_POLL_INTERVAL 500u
#define BOOT_PROG_GPIO_PIN 20u

typedef struct program {
    u32 size;
    u8 *code;
} program_t;

typedef struct program_header {
    u32 type;
    u32 itcm_size;
    u32 dtcm_size;
    u32 kernel_size;
    u32 initrd_size;
    u32 dtb_size;
    u32 kernel_load;
    u32 initrd_load;
    u32 dtb_load;
    u32 padding;
} program_header_t;

typedef struct sim_top {
    mod_t mod;
    itf_t uart_rx_itf;
    itf_t uart_tx_itf;
    AXI4_IF_DECL(ddr_);
    AXI4_IF_DECL(flash_);

    itf_t gpio_inout_itf;
    gpio_if_t *gpio_inout_io;

    ram_t ddr;
    rom_t flash;
    soc_t soc;

    bool end_sim;
    int exit_code;

    const char *prog_path;
    bool fast_load_linux;
    bool web_ui;
    bool no_end_detect;
    bool boot_prog;
    bool perf_sim;
    bool smp_opt;

    sim_ui_t *ui;
} sim_top_t;

static void sim_top_gpio_cb(void *args)
{
    sim_top_t *s = (sim_top_t *)args;
    u32 val = s->gpio_inout_io->val;
    DBG_LOG(LOG_INFO, "gpio: output = 0x%08x\n", val);
    DBG_CHECK(s->ui != NULL);
    s->ui->gpio_change(s->ui, val);
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

static inline u32 align4(u32 val)
{
    return (val + 3u) & ~3u;
}

static void sim_top_preload_linux(sim_top_t *sim_top, const program_t *program)
{
    if (program->size < LINUX_BIN_HEADER_SIZE) {
        return;
    }

    program_header_t *header = (program_header_t *)program->code;
    if (header->type != BIN_TYPE_LINUX) {
        return;
    }

    u32 itcm_size = header->itcm_size;
    u32 dtcm_size = header->dtcm_size;
    u32 kernel_size = header->kernel_size;
    u32 initrd_size = header->initrd_size;
    u32 dtb_size = header->dtb_size;
    u32 kernel_load = header->kernel_load;
    u32 initrd_load = header->initrd_load;
    u32 dtb_load = header->dtb_load;

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

    header->kernel_size = 0;
    header->initrd_size = 0;
    header->dtb_size = 0;

    DBG_LOG(LOG_PRINT, "sim_top: preloaded linux payloads to DDR: kernel %u bytes @ 0x%08x,"
        " initrd %u bytes @ 0x%08x, dtb %u bytes @ 0x%08x\n",
        kernel_size, kernel_load, initrd_size, initrd_load, dtb_size, dtb_load);
}

static void sim_top_burn_program(sim_top_t *sim_top)
{
    program_t program = read_program(sim_top->prog_path);
    DBG_CHECK(program.code != NULL);

    if (sim_top->fast_load_linux) {
        sim_top_preload_linux(sim_top, &program);
    }
    rom_burn(&sim_top->flash, program.code, 0, program.size);

    free(program.code);
}

static void sim_top_construct(sim_top_t *sim_top, const char *parent, const char *name,
    const char *prog_path, bool fast_load_linux, bool web_ui, bool no_end_detect,
    bool boot_prog, bool perf_sim, bool smp_opt)
{
    mod_construct(&sim_top->mod, parent, name);
    sim_top->mod.cycle = dbg_pcm_reg_perf_cnt(sim_top->mod.hier_name, "cycles");
    dbg_pcm_set_cycle(sim_top->mod.cycle);
    DBG_VCD_MODULE_SCOPE(name);

    UART_IF_CONSTRUCT(sim_top, uart_rx_itf, 1);
    UART_IF_CONSTRUCT(sim_top, uart_tx_itf, 1);
    AXI4_IF_CONSTRUCT(sim_top, ddr_, 1);
    AXI4_IF_CONSTRUCT(sim_top, flash_, 1);
    GPIO_SIGNAL_IF_CONSTRUCT(sim_top, gpio_inout_itf, false, false);

    sim_top->gpio_inout_io = itf_signal_get_src_and_chk(&sim_top->gpio_inout_itf);
    itf_signal_set_wcb(&sim_top->gpio_inout_itf, &sim_top_gpio_cb, sim_top);

    sim_top->soc.mod.cycle = sim_top->mod.cycle;
    AXI4_MST_CONNECT(&sim_top->soc, ddr_, sim_top, ddr_);
    AXI4_MST_CONNECT(&sim_top->soc, flash_, sim_top, flash_);
    sim_top->soc.uart_tx_mst = &sim_top->uart_rx_itf;
    sim_top->soc.uart_rx_slv = &sim_top->uart_tx_itf;
    sim_top->soc.gpio_inout = &sim_top->gpio_inout_itf;
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        sim_top->soc.ext_irq_ins[i] = NULL;
    }
    soc_construct(&sim_top->soc, sim_top->mod.hier_name, "u_soc",
        perf_sim, smp_opt);

    AXI4_SLV_CONNECT(&sim_top->ddr, , sim_top, ddr_);
    sim_top->ddr.mod.cycle = sim_top->mod.cycle;
    u32 ddr_latency = perf_sim ? DDR_LATENCY : 0u;
    ram_construct(&sim_top->ddr, sim_top->mod.hier_name, "u_ddr", 1,
        RAM_MODE_AXI, DDR_SIZE, DDR_BASE, ddr_latency);

    AXI4_SLV_CONNECT(&sim_top->flash, , sim_top, flash_);
    sim_top->flash.mod.cycle = sim_top->mod.cycle;
    rom_construct(&sim_top->flash, sim_top->mod.hier_name, "u_flash",
        ROM_MODE_AXI, FLASH_SIZE, NULL, 0, FLASH_BASE);

    dbg_vcd_set_clk(sim_top->mod.cycle);
    dbg_vcd_add_sig("cycle", DBG_SIG_TYPE_REG, 64, sim_top->mod.cycle);

    sim_top->prog_path = prog_path;
    sim_top->fast_load_linux = fast_load_linux;
    sim_top->web_ui = web_ui;
    sim_top->no_end_detect = no_end_detect;
    sim_top->boot_prog = boot_prog;
    sim_top->perf_sim = perf_sim;
    sim_top->smp_opt = smp_opt;

    sim_top_burn_program(sim_top);

    if (web_ui) {
        sim_top->ui = ui_web_create();
    } else {
        sim_top->ui = ui_term_create();
    }
}

static void sim_top_reset(sim_top_t *sim_top)
{
    mod_reset(&sim_top->mod);
    sim_top->end_sim = false;
    sim_top->exit_code = 0;
    DBG_CHECK(sim_top->ui != NULL);
    sim_top->ui->reset(sim_top->ui);
    soc_reset(&sim_top->soc);
    ram_reset(&sim_top->ddr);
    rom_reset(&sim_top->flash);
    dbg_vcd_reset();

    itf_reset(&sim_top->uart_rx_itf);
    itf_reset(&sim_top->uart_tx_itf);
    AXI4_IF_RESET(sim_top, ddr_);
    AXI4_IF_RESET(sim_top, flash_);
    itf_reset(&sim_top->gpio_inout_itf);

    u32 gpio_val = sim_top->ui->gpio_in_read(sim_top->ui);
    sim_top->gpio_inout_io->val = gpio_val;

    if (sim_top->boot_prog) {
        sim_top->gpio_inout_io->val |= (1u << BOOT_PROG_GPIO_PIN);
        sim_top->ui->gpio_change(sim_top->ui, sim_top->gpio_inout_io->val);
    }
}

static void sim_top_clock(sim_top_t *sim_top)
{
    mod_clock(&sim_top->mod);
    DBG_CHECK(sim_top->ui != NULL);

    u32 gpio_in;
    if (((*sim_top->mod.cycle) % GPIO_IN_POLL_INTERVAL) == 0u) {
        if (sim_top->ui->gpio_in_poll(sim_top->ui, &gpio_in)) {
            sim_top->gpio_inout_io->val =
                (sim_top->gpio_inout_io->val & 0xFFFFu) | (gpio_in & 0xFF0000u);
        }
    }

    u8 ch_rx;
    if (((*sim_top->mod.cycle) % UART_IN_POLL_INTERVAL) == 0u) {
        while (!itf_fifo_full(&sim_top->uart_tx_itf) &&
               sim_top->ui->uart_in(sim_top->ui, &ch_rx)) {
            uart_if_t pkt;
            pkt.data = ch_rx;
            itf_write(&sim_top->uart_tx_itf, &pkt);
        }
    }

    soc_clock(&sim_top->soc);
    ram_clock(&sim_top->ddr);
    rom_clock(&sim_top->flash);

    itf_dbg_clock(&sim_top->uart_rx_itf);
    itf_dbg_clock(&sim_top->uart_tx_itf);
    AXI4_IF_DBG_CLOCK(sim_top, ddr_);
    AXI4_IF_DBG_CLOCK(sim_top, flash_);
    itf_dbg_clock(&sim_top->gpio_inout_itf);

    (*(u64 *)sim_top->mod.cycle)++;
    dbg_pcm_clock();
    dbg_vcd_clock();

    i32 ch_tx;
    if (!itf_fifo_empty(&sim_top->uart_rx_itf)) {
        uart_if_t pkt;
        itf_read(&sim_top->uart_rx_itf, &pkt);

        ch_tx.u = pkt.data;
        if (!sim_top->no_end_detect && ch_tx.s == SIM_END_CHAR) {
            sim_top->exit_code = (int)sim_top->soc.cpu.hart.exu.gpr[5];
            sim_top->end_sim = true;
        } else {
            sim_top->ui->uart_out(sim_top->ui, (u8)ch_tx.s);
        }
    }

    if (sim_top->ui->reset_pending(sim_top->ui)) {
        sim_top_reset(sim_top);
        if (sim_top->fast_load_linux) {
            program_t program = read_program(sim_top->prog_path);
            DBG_CHECK(program.code != NULL);
            sim_top_preload_linux(sim_top, &program);
            free(program.code);
        }
    }
}

static void sim_top_free(sim_top_t *sim_top)
{
    mod_free(&sim_top->mod);
    DBG_CHECK(sim_top->ui != NULL);
    sim_top->ui->cleanup(sim_top->ui);

    soc_free(&sim_top->soc);
    ram_free(&sim_top->ddr);
    rom_free(&sim_top->flash);

    itf_free(&sim_top->uart_rx_itf);
    itf_free(&sim_top->uart_tx_itf);
    AXI4_IF_FREE(sim_top, ddr_);
    AXI4_IF_FREE(sim_top, flash_);
    itf_free(&sim_top->gpio_inout_itf);
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options] <program.bin>\n"
        "\n"
        "Options:\n"
        "  --fast-load-linux  Preload Linux payloads to DDR via fast path\n"
        "  --web-ui           Use web-based UI instead of terminal\n"
        "  --no-end-detect    Disable test-end detection (0x10 terminator)\n"
        "  --boot-prog        Enable bootloader progress output via UART\n"
        "  --perf-sim         Enable configured memory/cache latency modeling\n"
        "  --st-sim           Disable default SMP simulation optimization\n"
        "\n"
        "Arguments:\n"
        "  <program.bin>      Path to the binary program image\n",
        prog);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }
    bool fast_load_linux = false;
    bool web_ui = false;
    bool no_end_detect = false;
    bool boot_prog = false;
    bool perf_sim = false;
    bool st_sim = false;
    const char *prog_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fast-load-linux") == 0) {
            fast_load_linux = true;
        } else if (strcmp(argv[i], "--web-ui") == 0) {
            web_ui = true;
        } else if (strcmp(argv[i], "--no-end-detect") == 0) {
            no_end_detect = true;
        } else if (strcmp(argv[i], "--boot-prog") == 0) {
            boot_prog = true;
        } else if (strcmp(argv[i], "--perf-sim") == 0) {
            perf_sim = true;
        } else if (strcmp(argv[i], "--st-sim") == 0) {
            st_sim = true;
        } else if (!prog_path) {
            prog_path = argv[i];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }
    if (!prog_path) {
        print_usage(argv[0]);
        return 1;
    }

    bool smp_opt = !st_sim;
    if (smp_opt && !smp_opt_supported()) {
        fprintf(stderr, "warning: %s\n", smp_opt_unsupported_reason());
        smp_opt = false;
    }

    sim_top_t sim_top;
    sim_top_construct(&sim_top, NULL, "sim_top", prog_path,
                      fast_load_linux, web_ui, no_end_detect, boot_prog,
                      perf_sim, smp_opt);
    sim_top_reset(&sim_top);

    while (!sim_top.end_sim) {
        sim_top_clock(&sim_top);
    }

    int ret = sim_top.exit_code;
    sim_top_free(&sim_top);
    return ret;
}
