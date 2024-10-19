#include "soc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void uart_output_show(uart_output_t *output, bool *end_sim)
{
    if (output->valid) {
        output->valid = false;
        if (output->ch != 0x10) {
            putchar(output->ch);
        } else {
            *end_sim = true;
        }
    }
}

static inline bool get_bool_env(const char *key)
{
    const char *val = getenv(key);
    return (val != NULL && strcmp(val, "1") == 0);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }

    uart_output_t uart_output = { .valid = false };
    log_sys_t log_sys = { .trace = NULL };

    if (get_bool_env("GEN_TRACE")) {
        log_sys.trace = fopen("trace.txt", "w");
    }

    soc_t soc;
    soc_construct(&soc, &uart_output, &log_sys);
    soc_burn_program(&soc, argv[1]);
    soc_reset(&soc);

    bool end_sim = false;
    while (!end_sim) {
        rv32i_exec(&soc.cpu);
        uart_output_show(&uart_output, &end_sim);
    }

    if (log_sys.trace != NULL) {
        fclose(log_sys.trace);
        log_sys.trace = NULL;
    }

    soc_free(&soc);
    return 0;
}