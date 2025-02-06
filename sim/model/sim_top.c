#include "soc.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct program {
    u32 size;
    void *code;
} program_t;

typedef struct sim_top {
    u64 cycles;
    itf_t uart_rx_itf;
    soc_t soc;
    bool end_sim;
} sim_top_t;

static program_t read_program(const char *path)
{
    program_t program;

    FILE *fp = fopen(path, "r");
    fseek(fp, 0L, SEEK_END);
    program.size = (u32)ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    program.code = malloc(program.size);
    fread(program.code, 1, program.size, fp);
    fclose(fp);

    return program;
}

static void soc_burn_program(soc_t *soc, const char *path)
{
    program_t program = read_program(path);
    rom_burn(&soc->flash, &program.size, 0, 4);
    rom_burn(&soc->flash, program.code, 4, program.size);
    free(program.code);
}

static void sim_top_construct(sim_top_t *sim_top, const char *prog_path)
{
    sim_top->cycles = 0;
    itf_construct(&sim_top->uart_rx_itf, &sim_top->cycles,
        "uart_rx_itf", &uart_if_to_str, sizeof(uart_if_t), 1);

    sim_top->soc.uart_tx = &sim_top->uart_rx_itf;
    soc_construct(&sim_top->soc, &sim_top->cycles);
    soc_burn_program(&sim_top->soc, prog_path);

    sim_top->end_sim = false;
}

static void sim_top_reset(sim_top_t *sim_top)
{
    soc_reset(&sim_top->soc);
}

static void sim_top_clock(sim_top_t *sim_top)
{
    soc_clock(&sim_top->soc);

    if (itf_fifo_empty(&sim_top->uart_rx_itf)) {
        sim_top->cycles++;
        return;
    }

    uart_if_t uart_if;
    itf_read(&sim_top->uart_rx_itf, &uart_if);

    i32 ch;
    ch.u = uart_if.data;
    if (ch.s != 0x10) {
        putchar(ch.s);
    } else {
        sim_top->end_sim = true;
    }

    sim_top->cycles++;
}

static void sim_top_free(sim_top_t *sim_top)
{
    soc_free(&sim_top->soc);
    itf_free(&sim_top->uart_rx_itf);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    sim_top_t sim_top;

    sim_top_construct(&sim_top, argv[1]);
    sim_top_reset(&sim_top);

    while (!sim_top.end_sim) {
        sim_top_clock(&sim_top);
    }

    sim_top_free(&sim_top);

    return 0;
}