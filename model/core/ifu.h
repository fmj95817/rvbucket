#ifndef IFU_H
#define IFU_H

#include "base/types.h"
#include "base/itf.h"

typedef struct ifu {
    itf_t *fch_req_mst;
    itf_t *fch_rsp_slv;
    itf_t *ex_req_mst;
    itf_t *ex_rsp_slv;
    itf_t *fl_req_mst;
    itf_t *fl_rsp_slv;

    u32 pc;
    u32 reset_pc;

    bool fl_req_pend;
    bool fch_req_pend;

    struct {
        u32 base;
        u32 size;
    } boot_rom_info;
} ifu_t;

extern void ifu_construct(ifu_t *ifu, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size);
extern void ifu_reset(ifu_t *ifu);
extern void ifu_clock(ifu_t *ifu);
extern void ifu_free(ifu_t *ifu);

#endif