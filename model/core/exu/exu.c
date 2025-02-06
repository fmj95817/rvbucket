#include "exu.h"
#include "dbg.h"

static inline void print_split_line(bool newline)
{
    DBG_LOG(LOG_TRACE, newline ?
        "------------------------------------------------------------------------------\n\n" :
        "------------------------------------------------------------------------------\n"
    );
}

static inline void exu_dump(exu_t *exu, u32 pc)
{
    DBG_LOG(LOG_TRACE, "x0/zero  = %08x   x1/ra = %08x   x2/sp  = %08x   x3/gp  = %08x\n", exu->gpr[0], exu->gpr[1], exu->gpr[2], exu->gpr[3]);
    DBG_LOG(LOG_TRACE, "x4/tp    = %08x   x5/t0 = %08x   x6/t1  = %08x   x7/t2  = %08x\n", exu->gpr[4], exu->gpr[5], exu->gpr[6], exu->gpr[7]);
    DBG_LOG(LOG_TRACE, "x8/s0/fp = %08x   x9/s1 = %08x  x10/a0  = %08x  x11/a1  = %08x\n", exu->gpr[8], exu->gpr[9], exu->gpr[10], exu->gpr[11]);
    DBG_LOG(LOG_TRACE, "x12/a2   = %08x  x13/a3 = %08x  x14/a4  = %08x  x15/a5  = %08x\n", exu->gpr[12], exu->gpr[13], exu->gpr[14], exu->gpr[15]);
    DBG_LOG(LOG_TRACE, "x16/a6   = %08x  x17/a7 = %08x  x18/s2  = %08x  x19/s3  = %08x\n", exu->gpr[16], exu->gpr[17], exu->gpr[18], exu->gpr[19]);
    DBG_LOG(LOG_TRACE, "x20/s4   = %08x  x21/s5 = %08x  x22/s6  = %08x  x23/s7  = %08x\n", exu->gpr[20], exu->gpr[21], exu->gpr[22], exu->gpr[23]);
    DBG_LOG(LOG_TRACE, "x24/s8   = %08x  x25/s9 = %08x  x26/s10 = %08x  x27/s11 = %08x\n", exu->gpr[24], exu->gpr[25], exu->gpr[26], exu->gpr[27]);
    DBG_LOG(LOG_TRACE, "x28/t3   = %08x  x29/t4 = %08x  x30/t5  = %08x  x31/t6  = %08x\n", exu->gpr[28], exu->gpr[29], exu->gpr[30], exu->gpr[31]);
    DBG_LOG(LOG_TRACE, "pc = %08x\n", pc);
}

static void exu_proc_ex_req(exu_t *exu)
{
    if (exu->ldst_req_pend) {
        return;
    }

    if (itf_fifo_empty(exu->ex_req_slv)) {
        return;
    }

    ex_req_if_t ex_req;
    itf_read(exu->ex_req_slv, &ex_req);

    if (!itf_fifo_empty(exu->fl_req_slv)) {
        fl_req_if_t fl_req;
        itf_read(exu->fl_req_slv, &fl_req);

        fl_rsp_if_t fl_rsp;
        itf_write(exu->fl_rsp_mst, &fl_rsp);
        return;
    }

    if (ex_req.is_boot_code) {
        dbg_disable_log_module(LOG_TRACE);
    }

    typedef void (*ex_req_proc_t)(exu_t *, const ex_req_if_t *);
    static ex_req_proc_t procs[128] = {
        [OPCODE_LUI] = &misc_ex_req_proc,
        [OPCODE_AUIPC] = &misc_ex_req_proc,
        [OPCODE_JAL] = &branch_ex_req_proc,
        [OPCODE_JALR] = &branch_ex_req_proc,
        [OPCODE_BRANCH] = &branch_ex_req_proc,
        [OPCODE_LOAD] = &ldst_ex_req_proc,
        [OPCODE_STORE] = &ldst_ex_req_proc,
        [OPCODE_ALUI] = &alu_ex_req_proc,
        [OPCODE_ALU] = &alu_ex_req_proc,
        [OPCODE_MEM] = &sys_ex_req_proc,
        [OPCODE_SYSTEM] = &sys_ex_req_proc
    };

    ex_req_proc_t proc = procs[ex_req.inst.base.opcode];
    if (proc) {
        print_split_line(false);
        exu_dump(exu, ex_req.pc);
        proc(exu, &ex_req);
        print_split_line(true);
    } else {
        DBG_CHECK(0);
    }

    if (ex_req.is_boot_code) {
        dbg_enable_log_module(LOG_TRACE);
    }
}

static void exu_proc_ldst_rsp(exu_t *exu)
{
    if (itf_fifo_empty(exu->ldst_rsp_slv)) {
        return;
    }

    ldst_rsp_if_t ldst_rsp;
    itf_read(exu->ldst_rsp_slv, &ldst_rsp);
    DBG_CHECK(ldst_rsp.ok);

    ldst_biu_rsp_proc(exu, &ldst_rsp);
}

void exu_clock(exu_t *exu)
{
    exu_proc_ex_req(exu);
    exu_proc_ldst_rsp(exu);
}

void exu_construct(exu_t *exu) {}

void exu_reset(exu_t *exu)
{
    exu->gpr[0] = 0;
    for (int i = 1; i < 32; i++) {
        exu->gpr[i] = (u32)rand();
    }
    exu->ldst_req_pend = false;
}

void exu_free(exu_t *exu) {}