#include <stdio.h>
#include <string.h>
#include "soc.h"
#include "itf/bti_if.h"
#include "itf/axi4_if.h"
#include "itf/uart_if.h"
#include "itf/gpio_if.h"
#include "linux_loader.h"
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
#define UART_IN_POLL_INTERVAL 500u
#define GPIO_IN_POLL_INTERVAL 500u
#define BOOT_PROG_GPIO_PIN 20u

typedef struct sim_top_conf {
    const char *prog_path;
    bool fast_load_linux;
    bool web_ui;
    bool no_end_detect;
    bool boot_prog;
    bool perf_sim;
    bool smp_opt;
    ufshc_conf_t ufshc;
} sim_top_conf_t;

typedef struct sim_top {
    mod_t mod;
    u64 cycle_val;
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

    sim_top_conf_t conf;

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

static sim_linux_conf_t sim_top_linux_conf(const sim_top_t *sim_top)
{
    sim_linux_conf_t conf = {
        .ufshc_en = sim_top->conf.ufshc.en
    };
    return conf;
}

static void sim_top_burn_program(sim_top_t *sim_top)
{
    sim_program_t program = sim_program_read(sim_top->conf.prog_path);
    sim_linux_conf_t linux_conf = sim_top_linux_conf(sim_top);
    DBG_CHECK(program.code != NULL);

    if (sim_top->conf.fast_load_linux) {
        sim_linux_preload(&linux_conf, &sim_top->ddr, &program);
    } else {
        sim_linux_append_dtb(&linux_conf, &program);
    }
    rom_burn(&sim_top->flash, program.code, 0, program.size);

    sim_program_free(&program);
}

static void sim_top_construct(sim_top_t *sim_top, const char *parent, const char *name,
    const sim_top_conf_t *conf)
{
    DBG_CHECK(conf != NULL);
    DBG_CHECK(conf->prog_path != NULL);

    sim_top->cycle_val = 0;
    sim_top->mod.cycle = &sim_top->cycle_val;
    mod_construct(&sim_top->mod, parent, name);
    dbg_pcm_set_cycle(sim_top->mod.cycle);
    DBG_VCD_MODULE_SCOPE(name);
    sim_top->conf = *conf;

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
    soc_conf_t soc_conf = {
        .perf_sim = conf->perf_sim,
        .smp_opt = conf->smp_opt,
        .ufshc = conf->ufshc
    };
    soc_construct(&sim_top->soc, sim_top->mod.hier_name, "u_soc", &soc_conf);

    AXI4_SLV_CONNECT(&sim_top->ddr, , sim_top, ddr_);
    sim_top->ddr.mod.cycle = sim_top->mod.cycle;
    u32 ddr_latency = conf->perf_sim ? DDR_LATENCY : 0u;
    ram_construct(&sim_top->ddr, sim_top->mod.hier_name, "u_ddr", 1,
        RAM_MODE_AXI, DDR_SIZE, DDR_BASE, ddr_latency);

    AXI4_SLV_CONNECT(&sim_top->flash, , sim_top, flash_);
    sim_top->flash.mod.cycle = sim_top->mod.cycle;
    rom_construct(&sim_top->flash, sim_top->mod.hier_name, "u_flash",
        ROM_MODE_AXI, FLASH_SIZE, NULL, 0, FLASH_BASE);

    dbg_vcd_set_clk(sim_top->mod.cycle);
    dbg_vcd_add_sig("cycle", DBG_SIG_TYPE_REG, 64, sim_top->mod.cycle);

    sim_top_burn_program(sim_top);

    if (conf->web_ui) {
        sim_top->ui = ui_web_create();
    } else {
        sim_top->ui = ui_term_create();
    }
}

static void sim_top_reset(sim_top_t *sim_top)
{
    mod_reset(&sim_top->mod);
    sim_top->cycle_val = 0;
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

    if (sim_top->conf.boot_prog) {
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

    sim_top->cycle_val++;
    dbg_pcm_clock();
    dbg_vcd_clock();

    i32 ch_tx;
    if (!itf_fifo_empty(&sim_top->uart_rx_itf)) {
        uart_if_t pkt;
        itf_read(&sim_top->uart_rx_itf, &pkt);

        ch_tx.u = pkt.data;
        if (!sim_top->conf.no_end_detect && ch_tx.s == SIM_END_CHAR) {
            sim_top->exit_code = (int)sim_top->soc.cpu.hart.exu.gpr[5];
            sim_top->end_sim = true;
        } else {
            sim_top->ui->uart_out(sim_top->ui, (u8)ch_tx.s);
        }
    }

    if (sim_top->ui->reset_pending(sim_top->ui)) {
        sim_top_reset(sim_top);
        if (sim_top->conf.fast_load_linux) {
            sim_program_t program = sim_program_read(sim_top->conf.prog_path);
            sim_linux_conf_t linux_conf = sim_top_linux_conf(sim_top);
            DBG_CHECK(program.code != NULL);
            sim_linux_preload(&linux_conf, &sim_top->ddr, &program);
            sim_program_free(&program);
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
        "  --ufs-image <path> Enable UFSHC with a persistent backing image\n"
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
    const char *ufs_image = NULL;
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
        } else if (strcmp(argv[i], "--ufs-image") == 0) {
            if (++i >= argc || ufs_image != NULL) {
                print_usage(argv[0]);
                return 1;
            }
            ufs_image = argv[i];
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

    sim_top_conf_t conf = {
        .prog_path = prog_path,
        .fast_load_linux = fast_load_linux,
        .web_ui = web_ui,
        .no_end_detect = no_end_detect,
        .boot_prog = boot_prog,
        .perf_sim = perf_sim,
        .smp_opt = smp_opt,
        .ufshc = {
            .en = ufs_image != NULL,
            .backing_path = ufs_image
        }
    };
    sim_top_t sim_top;
    sim_top_construct(&sim_top, NULL, "sim_top", &conf);
    sim_top_reset(&sim_top);

    while (!sim_top.end_sim) {
        sim_top_clock(&sim_top);
    }

    int ret = sim_top.exit_code;
    sim_top_free(&sim_top);
    return ret;
}
