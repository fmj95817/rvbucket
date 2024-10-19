#ifndef TRANS_H
#define TRANS_H

#include "types.h"
#include "isa.h"
#include "bus.h"

typedef struct ifu2exu_trans {
    struct {
        rv32i_inst_t inst;
        u32 pc;
    } req;
    struct {
        bool taken;
        bool abs_jump;
        u32 pc_offset;
        u32 target_pc;
    } rsp;
} ifu2exu_trans_t;

typedef struct ifu2lsu_trans {
    struct {
        u32 pc;
    } req;
    struct {
        u32 ir;
        bool ok;
    } rsp;
} ifu2lsu_trans_t;

typedef struct exu2lsu_trans {
    bus_req_t req;
    bus_rsp_t rsp;
} exu2lsu_trans_t;

#endif