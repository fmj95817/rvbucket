#include "soc.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct program {
    u32 size;
    void *code;
} program_t;

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

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    u64 cycles = 0;

    itf_t uart_itf;
    itf_construct(&uart_itf, &cycles, "uart_itf", &uart_if_to_str, sizeof(uart_if_t), 1);

    soc_t soc;
    soc.uart_mst = &uart_itf;
    soc_construct(&soc, &cycles);

    soc_burn_program(&soc, argv[1]);
    soc_reset(&soc);

    while (true) {
        soc_clock(&soc);
        cycles++;
        if (itf_fifo_empty(&uart_itf)) {
            continue;
        }

        uart_if_t uart_data;
        itf_read(&uart_itf, &uart_data);
        if (uart_data.tx != 0x10) {
            putchar(uart_data.tx);
        } else {
            break;
        }
    }

    printf("cycles: %lu\n", cycles);

    soc_free(&soc);
    itf_free(&uart_itf);
    return 0;
}