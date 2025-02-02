#ifndef MOD_IF_H
#define MOD_IF_H

#include "base/types.h"
#include "isa.h"

typedef struct ex_req_if {
    rv32i_inst_t inst;
    u32 pc;
    bool is_boot_code;
} ex_req_if_t;

typedef struct ex_rsp_if {
    bool taken;
    u32 target_pc;
} ex_rsp_if_t;

typedef struct fch_req_if {
    u32 pc;
} fch_req_if_t;

typedef struct fch_rsp_if {
    u32 ir;
    bool ok;
} fch_rsp_if_t;

typedef struct fl_req_if {
    u32 dummy;
} fl_req_if_t;

typedef struct fl_rsp_if {
    u32 dummy;
} fl_rsp_if_t;

typedef struct ldst_req_if {
    u32 addr;
    bool st;
    u32 data;
    u8 strobe;
} ldst_req_if_t;

typedef struct ldst_rsp_if {
    u32 data;
    bool ok;
} ldst_rsp_if_t;

#endif