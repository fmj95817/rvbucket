#ifndef EXU_H
#define EXU_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/ex_req_if.h"
#include "itf/ex_rsp_if.h"
#include "itf/fl_req_if.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/hart_expt_if.h"
#include "itf/exu_csr_read_req_if.h"
#include "itf/csr_exu_read_rsp_if.h"
#include "itf/exu_csr_write_req_if.h"
#include "itf/csr_exu_write_rsp_if.h"
#include "itf/tlb_flush_if.h"
#include "itf/l1_flush_if.h"
#include "itf/exu_state_if.h"
#include "itf/trap_exu_ctrl_if.h"
#include "spec/core/isa.h"

typedef enum amo_stage {
    AMO_STAGE_IDLE = 0,
    AMO_STAGE_READ = 1,
    AMO_STAGE_WRITE = 2
} amo_stage_t;

typedef struct exu {
    mod_t mod;
    itf_t *fl_req_slv;
    itf_t *ex_req_slv;
    itf_t *ex_rsp_mst;
    itf_t *ldst_req_mst;
    itf_t *ldst_rsp_slv;
    itf_t *ex_expt_mst;
    itf_t *exu_csr_read_req_out;
    itf_t *csr_exu_read_rsp_in;
    itf_t *exu_csr_write_req_out;
    itf_t *csr_exu_write_rsp_in;
    itf_t *tlb_flush_mst;
    itf_t *l1i_flush_mst;
    itf_t *exu_state_out;
    itf_t *trap_exu_ctrl_in;

    exu_csr_read_req_if_t *csr_read_req_o;
    const csr_exu_read_rsp_if_t *csr_read_rsp_i;
    exu_csr_write_req_if_t *csr_write_req_o;
    const csr_exu_write_rsp_if_t *csr_write_rsp_i;
    exu_state_if_t *state_o;
    const trap_exu_ctrl_if_t *trap_ctrl_i;

    rv32g_priv_t priv;
    u32 cur_pc;
    u32 irq_epc;
    bool irq_defer;

    u32 cur_opcode;

    bool ldst_req_pend;
    u32 ld_rd;
    u32 ld_funct3;

    u32 amo_stage;
    u32 amo_rd;
    u32 amo_addr;
    i32 amo_s2;
    u32 amo_funct375;
    bool amo_lr_set;
    u32 amo_rsvd_addr;

    bool wfi;
    u32 wfi_resume_pc;

    u32 gpr[RV32G_GPR_NUM];
} exu_t;

extern void exu_construct(exu_t *exu, const char *parent, const char *name);
extern void exu_reset(exu_t *exu);
extern void exu_clock(exu_t *exu);
extern void exu_free(exu_t *exu);

extern void ldst_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void alu_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void branch_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void misc_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void mem_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void sys_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void amo_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);

extern void ldst_biu_rsp_proc(exu_t *exu, const ldst_rsp_if_t *ldst_rsp);
extern void amo_biu_rsp_proc(exu_t *exu, const ldst_rsp_if_t *ldst_rsp);

#endif
