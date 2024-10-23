#ifndef MOD_IF_H
#define MOD_IF_H

#include "types.h"
#include "isa.h"
#include "bus.h"

typedef struct iexec_if {
    struct {
        rv32i_inst_t inst;
        u32 pc;
        bool is_boot_code;
    } req;
    struct {
        bool taken;
        bool abs_jump;
        u32 pc_offset;
        u32 target_pc;
    } rsp;
} iexec_if_t;

typedef struct ifetch_if {
    struct {
        u32 pc;
    } req;
    struct {
        u32 ir;
        bool ok;
    } rsp;
} ifetch_if_t;

typedef struct ldst_if {
    struct {
        u32 addr;
        bool st;
        u32 data;
        u8 strobe;
    } req;
    struct {
        u32 data;
        bool ok;
    } rsp;
} ldst_if_t;

#endif