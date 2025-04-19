#ifndef SOC_H
#define SOC_H

#include "base/types.h"
#include "core/rv32i.h"
#include "mem/ram.h"
#include "mem/rom.h"
#include "io/uart.h"
#include "bti.h"

typedef struct soc {
    itf_t *uart_tx;

    rv32i_t cpu;
    rom_t boot_rom;
    rom_t flash;
    ram_t itcm;
    ram_t dtcm;
    uart_t uart;
    bti_demux_t i_bti_demux;
    bti_demux_t d_bti_demux;

    itf_t cpu_i_bti_req_itf;
    itf_t cpu_i_bti_rsp_itf;
    itf_t cpu_d_bti_req_itf;
    itf_t cpu_d_bti_rsp_itf;
    itf_t boot_rom_bti_req_itf;
    itf_t boot_rom_bti_rsp_itf;
    itf_t flash_bti_req_itf;
    itf_t flash_bti_rsp_itf;
    itf_t itcm_i_bti_req_itf;
    itf_t itcm_i_bti_rsp_itf;
    itf_t itcm_d_bti_req_itf;
    itf_t itcm_d_bti_rsp_itf;
    itf_t dtcm_bti_req_itf;
    itf_t dtcm_bti_rsp_itf;
    itf_t uart_bti_req_itf;
    itf_t uart_bti_rsp_itf;
} soc_t;

extern void soc_construct(soc_t *soc, const u64 *cycle);
extern void soc_reset(soc_t *soc);
extern void soc_clock(soc_t *soc);
extern void soc_free(soc_t *soc);

#endif