#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "soc.h"
#include "itf/uart_if.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

#define SIM_END_CHAR 0x10

typedef struct program {
    u32 size;
    void *code;
} program_t;

#define STDIN_BUF_SIZE 256

typedef struct sim_top {
    u64 *cycle;
    itf_t uart_rx_itf;
    itf_t uart_tx_itf;
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

    FILE *fp = fopen(path, "r");
    fseek(fp, 0L, SEEK_END);
    program.size = (u32)ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    program.code = malloc(program.size);
    (void)!fread(program.code, 1, program.size, fp);
    fclose(fp);

    return program;
}

static void soc_burn_program(soc_t *soc, const char *path)
{
    program_t program = read_program(path);
    rom_burn(&soc->flash, program.code, 0, program.size);
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

static void sim_top_construct(sim_top_t *sim_top, const char *name, const char *prog_path)
{
    DBG_VCD_MODULE_SCOPE(name);

    sim_top->cycle = dbg_pcm_reg_perf_cnt("cycles");

    UART_IF_CONSTRUCT(sim_top, uart_rx_itf, 1);
    UART_IF_CONSTRUCT(sim_top, uart_tx_itf, 1);

    sim_top->soc.cycle = sim_top->cycle;
    sim_top->soc.ddr_bti_req_mst = NULL;
    sim_top->soc.ddr_bti_rsp_slv = NULL;
    sim_top->soc.uart_tx_mst = &sim_top->uart_rx_itf;
    sim_top->soc.uart_rx_slv = &sim_top->uart_tx_itf;
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        sim_top->soc.ext_irq_ins[i] = NULL;
    }
    soc_construct(&sim_top->soc, "u_soc");

    dbg_vcd_set_clk(sim_top->cycle);
    dbg_vcd_add_sig("cycle", DBG_SIG_TYPE_REG, 64, sim_top->cycle);

    soc_burn_program(&sim_top->soc, prog_path);
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
    itf_dbg_clock(&sim_top->uart_rx_itf);
    itf_dbg_clock(&sim_top->uart_tx_itf);

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
    itf_free(&sim_top->uart_rx_itf);
    itf_free(&sim_top->uart_tx_itf);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    sim_top_t sim_top;

    sim_top_construct(&sim_top, "sim_top", argv[1]);
    sim_top_reset(&sim_top);

    while (!sim_top.end_sim) {
        sim_top_clock(&sim_top);
    }

    sim_top_free(&sim_top);

    return 0;
}
