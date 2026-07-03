#ifndef TRAP_H
#define TRAP_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/hart_expt_if.h"
#include "itf/trap_send_if.h"
#include "itf/exu_state_if.h"
#include "itf/trap_exu_ctrl_if.h"
#include "itf/csr_trap_state_if.h"
#include "itf/trap_csr_write_req_if.h"
#include "itf/csr_trap_write_rsp_if.h"
#include "spec/core/csr.h"

typedef struct trap {
    mod_t mod;
    itf_t *fch_expt_slv;
    itf_t *ex_expt_slv;
    itf_t *ldst_expt_slv;
    itf_t *trap_send_mst;
    itf_t *exu_state_in;
    itf_t *trap_exu_ctrl_out;
    itf_t *csr_trap_state_in;
    itf_t *trap_csr_write_req_out;
    itf_t *csr_trap_write_rsp_in;

    const exu_state_if_t *exu_state_i;
    trap_exu_ctrl_if_t *exu_ctrl_o;
    const csr_trap_state_if_t *csr_state_i;
    trap_csr_write_req_if_t *csr_write_req_o;
    const csr_trap_write_rsp_if_t *csr_write_rsp_i;
} trap_t;

extern void trap_construct(trap_t *trap, const char *parent, const char *name);
extern void trap_reset(trap_t *trap);
extern void trap_clock(trap_t *trap);
extern void trap_free(trap_t *trap);

#endif
