#ifndef IFU_H
#define IFU_H

#include "base/types.h"
#include "base/itf.h"

#define IFU_BHT_SIZE 16

typedef struct ifu {
    itf_t *fch_req_mst;
    itf_t *fch_rsp_slv;
    itf_t *ex_req_mst;
    itf_t *ex_rsp_slv;
    itf_t *fl_req_mst;

    u32 reset_pc;

    struct {
        bool pend;
        bool vld;
        u32 pc;
        u32 ir;
    } fch;

    struct {
        bool vld;
        u32 pc;
        u32 ir;
        u32 pred_target_pc;
        u32 pred_taken;
    } issue;

    struct {
        bool vld;
        u32 pc;
    } resume;

    struct {
        bool enable;
        struct {
            bool vld;
            u32 pc;
            bool taken;
            u32 target_pc;
            u16 used_bits;
        } bht[IFU_BHT_SIZE];
    } bpu;

    struct {
        u32 base;
        u32 size;
    } boot_rom_info;

    struct {
        u64 *branch;
        u64 *pred_true;
    } perf;
} ifu_t;

extern void ifu_construct(ifu_t *ifu, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size);
extern void ifu_reset(ifu_t *ifu);
extern void ifu_clock(ifu_t *ifu);
extern void ifu_free(ifu_t *ifu);

#endif