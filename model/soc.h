#ifndef SOC_H
#define SOC_H

#include "types.h"
#include "core/rv32i.h"
#include "mem/ram.h"
#include "mem/rom.h"
#include "io/uart.h"
#include "dbg.h"

typedef struct soc {
    rv32i_t cpu;
    rom_t boot_rom;
    rom_t flash;
    ram_t tcm;
    uart_t uart;
} soc_t;

extern void soc_construct(soc_t *soc, uart_output_t *uart_output);
extern void soc_reset(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif