#ifndef RV32I_H
#define RV32I_H

#include "base/types.h"
#include "base/itf.h"
#include "mod_if.h"
#include "ifu.h"
#include "biu.h"
#include "exu/exu.h"

typedef struct rv32i {
    ifu_t ifu;
    exu_t exu;
    biu_t biu;

    itf_t *bti_req_mst;
    itf_t *bti_rsp_slv;

    itf_t fch_req_itf;
    itf_t fch_rsp_itf;
    itf_t ex_req_itf;
    itf_t ex_rsp_itf;
    itf_t fl_req_itf;
    itf_t fl_rsp_itf;
    itf_t ldst_req_itf;
    itf_t ldst_rsp_itf;
} rv32i_t;

extern void rv32i_construct(rv32i_t *s, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size);
extern void rv32i_reset(rv32i_t *s);
extern void rv32i_clock(rv32i_t *s);
extern void rv32i_free(rv32i_t *s);

#endif