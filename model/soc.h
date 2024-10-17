#ifndef SOC_H
#define SOC_H

#include "types.h"
#include "core/rv32i.h"
#include "mem/ram.h"
#include "mem/rom.h"

typedef struct soc {
    rv32i_t cpu;
    ram_t tcm;
    rom_t flash;
} soc_t;

extern void soc_construct(soc_t *soc);
extern void soc_reset(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif