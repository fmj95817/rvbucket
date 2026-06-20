#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include "soc.h"
#include "mem/ram.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/uart_if.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"
#include "spec/soc.h"

#define SIM_END_CHAR 0x10
#define BIN_TYPE_LINUX 1u
#define LINUX_BIN_HEADER_SIZE 40u

typedef struct program {
    u32 size;
    u8 *code;
} program_t;

#define STDIN_BUF_SIZE 256

typedef struct sim_top {
    u64 *cycle;
    itf_t uart_rx_itf;
    itf_t uart_tx_itf;
    itf_t ddr_bti_req_itf;
    itf_t ddr_bti_rsp_itf;
    ram_t ddr;
    soc_t soc;
    bool end_sim;
    struct termios orig_termios;
    bool tty_raw;

    char stdin_buf[STDIN_BUF_SIZE];
    u32 stdin_rd;
    u32 stdin_wr;
} sim_top_t;

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
    return ((u32)data[0]) |
        ((u32)data[1] << 8u) |
        ((u32)data[2] << 16u) |
        ((u32)data[3] << 24u);
}

static void write_u32_le(u8 *data, u32 val)
{
    data[0] = val & 0xffu;
    data[1] = (val >> 8u) & 0xffu;
    data[2] = (val >> 16u) & 0xffu;
    data[3] = (val >> 24u) & 0xffu;
}

static u32 align4(u32 val)
{
    return (val + 3u) & ~3u;
}

static void sim_top_preload_linux(sim_top_t *sim_top, program_t *program)
{
    if (program->size < LINUX_BIN_HEADER_SIZE) {
        return;
    }
    if (read_u32_le(program->code) != BIN_TYPE_LINUX) {
        return;
    }

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
    if (fast_load_linux) {
        sim_top_preload_linux(sim_top, &program);
    }
    rom_burn(&sim_top->soc.flash, program.code, 0, program.size);
    free(program.code);
}

static void sim_top_tty_raw_enable(sim_top_t *sim_top)
{
    if (isatty(STDIN_FILENO)) {
        tcgetattr(STDIN_FILENO, &sim_top->orig_termios);
        struct termios raw = sim_top->orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        sim_top->tty_raw = true;
    } else {
        sim_top->tty_raw = false;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

static void sim_top_tty_raw_disable(sim_top_t *sim_top)
{
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    if (sim_top->tty_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &sim_top->orig_termios);
    }
}

static void sim_top_construct(sim_top_t *sim_top, const char *name,
    const char *prog_path, bool fast_load_linux)
{
    DBG_VCD_MODULE_SCOPE(name);

    sim_top->cycle = dbg_pcm_reg_perf_cnt("cycles");

    UART_IF_CONSTRUCT(sim_top, uart_rx_itf, 1);
    UART_IF_CONSTRUCT(sim_top, uart_tx_itf, 1);
    BTI_REQ_IF_CONSTRUCT(sim_top, ddr_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(sim_top, ddr_bti_rsp_itf, 1);

    sim_top->soc.cycle = sim_top->cycle;
    sim_top->soc.ddr_bti_req_mst = &sim_top->ddr_bti_req_itf;
    sim_top->soc.ddr_bti_rsp_slv = &sim_top->ddr_bti_rsp_itf;
    sim_top->soc.uart_tx_mst = &sim_top->uart_rx_itf;
    sim_top->soc.uart_rx_slv = &sim_top->uart_tx_itf;
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        sim_top->soc.ext_irq_ins[i] = NULL;
    }
    soc_construct(&sim_top->soc, "u_soc");

    sim_top->ddr.bti_req_slv[0] = &sim_top->ddr_bti_req_itf;
    sim_top->ddr.bti_rsp_mst[0] = &sim_top->ddr_bti_rsp_itf;
    ram_construct(&sim_top->ddr, "u_ddr", 1, DDR_SIZE, DDR_BASE);

    dbg_vcd_set_clk(sim_top->cycle);
    dbg_vcd_add_sig("cycle", DBG_SIG_TYPE_REG, 64, sim_top->cycle);

    soc_burn_program(sim_top, prog_path, fast_load_linux);
}

static void sim_top_reset(sim_top_t *sim_top)
{
    sim_top->end_sim = false;
    sim_top->stdin_rd = 0;
    sim_top->stdin_wr = 0;
    sim_top_tty_raw_enable(sim_top);
    soc_reset(&sim_top->soc);
    dbg_vcd_reset();
}

static void sim_top_poll_stdin(sim_top_t *sim_top)
{
    while (1) {
        u32 next_wr = (sim_top->stdin_wr + 1) % STDIN_BUF_SIZE;
        if (next_wr == sim_top->stdin_rd)
            break;
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n <= 0)
            break;
        sim_top->stdin_buf[sim_top->stdin_wr] = ch;
        sim_top->stdin_wr = next_wr;
    }

    if (sim_top->stdin_rd == sim_top->stdin_wr)
        return;
    if (itf_fifo_full(&sim_top->uart_tx_itf))
        return;

    uart_if_t pkt;
    pkt.data = (u32)(unsigned char)sim_top->stdin_buf[sim_top->stdin_rd];
    itf_write(&sim_top->uart_tx_itf, &pkt);
    sim_top->stdin_rd = (sim_top->stdin_rd + 1) % STDIN_BUF_SIZE;
}

static void sim_top_clock(sim_top_t *sim_top)
{
    sim_top_poll_stdin(sim_top);
    soc_clock(&sim_top->soc);
    ram_clock(&sim_top->ddr);
    itf_dbg_clock(&sim_top->uart_rx_itf);
    itf_dbg_clock(&sim_top->uart_tx_itf);
    itf_dbg_clock(&sim_top->ddr_bti_req_itf);
    itf_dbg_clock(&sim_top->ddr_bti_rsp_itf);

    (*sim_top->cycle)++;
    dbg_vcd_clock();

    if (itf_fifo_empty(&sim_top->uart_rx_itf)) {
        return;
    }

    uart_if_t uart_if;
    itf_read(&sim_top->uart_rx_itf, &uart_if);

    i32 ch;
    ch.u = uart_if.data;
    if (ch.s != SIM_END_CHAR) {
        putchar(ch.s);
        fflush(stdout);
    } else {
        sim_top->end_sim = true;
    }
}

static void sim_top_free(sim_top_t *sim_top)
{
    sim_top_tty_raw_disable(sim_top);
    soc_free(&sim_top->soc);
    ram_free(&sim_top->ddr);
    itf_free(&sim_top->uart_rx_itf);
    itf_free(&sim_top->uart_tx_itf);
    itf_free(&sim_top->ddr_bti_req_itf);
    itf_free(&sim_top->ddr_bti_rsp_itf);
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

    sim_top_free(&sim_top);

    return 0;
}
