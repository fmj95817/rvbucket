#ifndef SOC_H
#define SOC_H

#include "base/types.h"
#include "core/rv32i.h"
#include "mem/ram.h"
#include "mem/rom.h"
#include "io/uart.h"
#include "bti.h"
#include "dbg.h"

typedef struct soc {
    itf_t *uart_tx;

    rv32i_t cpu;
    rom_t boot_rom;
    rom_t flash;
    ram_t tcm;
    uart_t uart;
    bti_mux_t bti_mux;

    itf_t cpu_bti_req_itf;
    itf_t cpu_bti_rsp_itf;
    itf_t boot_rom_bti_req_itf;
    itf_t boot_rom_bti_rsp_itf;
    itf_t flash_bti_req_itf;
    itf_t flash_bti_rsp_itf;
    itf_t tcm_bti_req_itf;
    itf_t tcm_bti_rsp_itf;
    itf_t uart_bti_req_itf;
    itf_t uart_bti_rsp_itf;
} soc_t;

extern void soc_construct(soc_t *soc, u64 *cycles);
extern void soc_reset(soc_t *soc);
extern void soc_clock(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif