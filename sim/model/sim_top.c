#include "soc.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct program {
    size_t size;
    void *code;
} program_t;

static program_t read_program(const char *path)
{
    FILE *fp = fopen(path, "r");
    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    void *code = malloc(size);
    fread(code, 1, size, fp);
    fclose(fp);

    program_t program = {
        .size = size,
        .code = code
    };

    return program;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    program_t program = read_program(argv[1]);

    soc_t soc;
    soc_construct(&soc);

    ram_load(&soc.tcm, program.code, 0, program.size);
    free(program.code);

    soc_reset(&soc);

    while (!uart_end_sim(&soc.uart)) {
        rv32i_exec(&soc.cpu);
    }

    soc_free(&soc);
    return 0;
}