#include "exu.h"
#include "utils.h"
#include "spec/core/csr.h"
#include "dbg/chk.h"
#include "dbg/log.h"

#define DECL_SYS_HANDLER(inst) static void inst##_ex_req_handler(exu_t *exu, const ex_req_if_t *req)
#define CALL_SYS_HANDLER(inst, exu, req) inst##_ex_req_handler(exu, req)
#define GET_SYS_HANDLER(inst) &inst##_ex_req_handler

typedef void (*sys_ex_req_handler_t)(exu_t *exu, const ex_req_if_t *req);

static void exu_send_expt(exu_t *exu, hart_expt_type_t type, hart_expt_cause_t cause, u32 pc, u32 tval)
{
    hart_expt_if_t pkt;
    pkt.type = type;
    pkt.cause = cause;
    pkt.priv = exu->priv;
    pkt.pc = pc;
    pkt.tval = tval;
    itf_write(exu->ex_expt_mst, &pkt);
}

DECL_SYS_HANDLER(ecall)
{
    DBG_LOG(LOG_TRACE, "ecall\n");
    exu_send_expt(exu, HART_EXPT_TYPE_EXCEPTION, HART_EXPT_CAUSE_ECALL, req->pc, 0);
}

DECL_SYS_HANDLER(ebreak)
{
    DBG_LOG(LOG_TRACE, "ebreak\n");
    exu_send_expt(exu, HART_EXPT_TYPE_EXCEPTION, HART_EXPT_CAUSE_BREAKPOINT, req->pc, req->pc);
}

DECL_SYS_HANDLER(sret)
{
    DBG_LOG(LOG_TRACE, "sret\n");
    exu_send_expt(exu, HART_EXPT_TYPE_SRET, (hart_expt_cause_t)0, req->pc, 0);
}

DECL_SYS_HANDLER(mret)
{
    DBG_LOG(LOG_TRACE, "mret\n");
    exu_send_expt(exu, HART_EXPT_TYPE_MRET, (hart_expt_cause_t)0, req->pc, 0);
}

DECL_SYS_HANDLER(wfi)
{
    DBG_LOG(LOG_TRACE, "wfi\n");
    DBG_LOG(LOG_TRAP, "wfi enter pc=%08x priv=%u\n", req->pc, (u32)exu->priv);
    exu->wfi = true;
    exu->wfi_resume_pc = req->pc + 4;
}

DECL_SYS_HANDLER(sfence_vma)
{
    DBG_LOG(LOG_TRACE, "sfence.vma\n");
    DBG_CHECK(!itf_fifo_full(exu->tlb_flush_mst));
    tlb_flush_if_t pkt = {};
    itf_write(exu->tlb_flush_mst, &pkt);
}

DECL_SYS_HANDLER(priv_group)
{
    u32 i_imm = req->inst.i.imm_11_0;
    u32 i_rs1 = req->inst.i.rs1;
    u32 i_rd = req->inst.i.rd;
    u32 r_funct7 = req->inst.r.funct7;
    u32 r_rs2 = req->inst.r.rs2;
    u32 r_rs1 = req->inst.r.rs1;
    u32 r_rd = req->inst.r.rd;

    if (i_imm == 0b000000000000 && i_rs1 == 0b00000 && i_rd == 0b00000) {
        CALL_SYS_HANDLER(ecall, exu, req);
    } else if (i_imm == 0b000000000001 && i_rs1 == 0b00000 && i_rd == 0b00000) {
        CALL_SYS_HANDLER(ebreak, exu, req);
    } else if (r_funct7 == 0b0001000 && r_rs2 == 0b00010 && r_rs1 == 0b00000 && r_rd == 0b00000) {
        CALL_SYS_HANDLER(sret, exu, req);
    } else if (r_funct7 == 0b0011000 && r_rs2 == 0b00010 && r_rs1 == 0b00000 && r_rd == 0b00000) {
        CALL_SYS_HANDLER(mret, exu, req);
    } else if (r_funct7 == 0b0001000 && r_rs2 == 0b00101 && r_rs1 == 0b00000 && r_rd == 0b00000) {
        CALL_SYS_HANDLER(wfi, exu, req);
    } else if (r_funct7 == 0b0001001 && r_rd == 0b00000) {
        CALL_SYS_HANDLER(sfence_vma, exu, req);
    } else {
        DBG_CHECK(0);
    }
}

static bool csr_read(exu_t *exu, u32 addr, u32 *data)
{
    exu->csr_read_req_o->addr = addr;
    exu->csr_read_req_o->priv = exu->priv;
    itf_signal_write_notify(exu->exu_csr_read_req_out);

    *data = exu->csr_read_rsp_i->val;
    return exu->csr_read_rsp_i->ok;
}

static bool csr_write(exu_t *exu, u32 addr, u32 data)
{
    exu->csr_write_req_o->addr = addr;
    exu->csr_write_req_o->priv = exu->priv;
    exu->csr_write_req_o->val = data;
    itf_signal_write_notify(exu->exu_csr_write_req_out);

    return exu->csr_write_rsp_i->ok;
}

static void csr_access_trap(exu_t *exu, const ex_req_if_t *req)
{
    exu_send_expt(exu, HART_EXPT_TYPE_EXCEPTION,
        HART_EXPT_CAUSE_ILLEGAL_INST, req->pc, req->inst.raw);
}

DECL_SYS_HANDLER(csrrw)
{
    u32 old_val = 0;
    u32 addr = req->inst.i.imm_11_0;

    if (req->inst.i.rd != 0 && !csr_read(exu, addr, &old_val)) {
        csr_access_trap(exu, req);
        return;
    }

    u32 new_val = get_gpr(exu, req->inst.i.rs1);
    if (!csr_write(exu, addr, new_val)) {
        csr_access_trap(exu, req);
        return;
    }
    set_gpr(exu, req->inst.i.rd, old_val);

    DBG_LOG(LOG_TRACE, "csrrw, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(addr), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrs)
{
    u32 old_val;
    u32 addr = req->inst.i.imm_11_0;

    if (!csr_read(exu, addr, &old_val)) {
        csr_access_trap(exu, req);
        return;
    }

    u32 mask = get_gpr(exu, req->inst.i.rs1);
    if (mask != 0) {
        if (!csr_write(exu, addr, old_val | mask)) {
            csr_access_trap(exu, req);
            return;
        }
    }
    set_gpr(exu, req->inst.i.rd, old_val);
    DBG_LOG(LOG_TRACE, "csrrs, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(addr), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrc)
{
    u32 old_val;
    u32 addr = req->inst.i.imm_11_0;

    if (!csr_read(exu, addr, &old_val)) {
        csr_access_trap(exu, req);
        return;
    }

    u32 mask = get_gpr(exu, req->inst.i.rs1);
    if (mask != 0) {
        if (!csr_write(exu, addr, old_val & (~mask))) {
            csr_access_trap(exu, req);
            return;
        }
    }
    set_gpr(exu, req->inst.i.rd, old_val);
    DBG_LOG(LOG_TRACE, "csrrc, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(addr), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrwi)
{
    u32 old_val = 0;
    u32 addr = req->inst.i.imm_11_0;

    if (req->inst.i.rd != 0 && !csr_read(exu, addr, &old_val)) {
        csr_access_trap(exu, req);
        return;
    }

    u32 new_val = req->inst.i.rs1;
    if (!csr_write(exu, addr, new_val)) {
        csr_access_trap(exu, req);
        return;
    }
    set_gpr(exu, req->inst.i.rd, old_val);

    DBG_LOG(LOG_TRACE, "csrrwi, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(addr), req->inst.i.rs1);
}

DECL_SYS_HANDLER(csrrsi)
{
    u32 old_val;
    u32 addr = req->inst.i.imm_11_0;

    if (!csr_read(exu, addr, &old_val)) {
        csr_access_trap(exu, req);
        return;
    }

    u32 mask = req->inst.i.rs1;
    if (mask != 0) {
        if (!csr_write(exu, addr, old_val | mask)) {
            csr_access_trap(exu, req);
            return;
        }
    }
    set_gpr(exu, req->inst.i.rd, old_val);
    DBG_LOG(LOG_TRACE, "csrrsi, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(addr), req->inst.i.rs1);
}

DECL_SYS_HANDLER(csrrci)
{
    u32 old_val;
    u32 addr = req->inst.i.imm_11_0;

    if (!csr_read(exu, addr, &old_val)) {
        csr_access_trap(exu, req);
        return;
    }

    u32 mask = req->inst.i.rs1;
    if (mask != 0) {
        if (!csr_write(exu, addr, old_val & (~mask))) {
            csr_access_trap(exu, req);
            return;
        }
    }
    set_gpr(exu, req->inst.i.rd, old_val);
    DBG_LOG(LOG_TRACE, "csrrci, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(addr), req->inst.i.rs1);
}

DECL_SYS_HANDLER(sys_group)
{
    static sys_ex_req_handler_t sys_handlers[8] = {
        [SYSTEM_FUNCT3_PRIV] = GET_SYS_HANDLER(priv_group),
        [SYSTEM_FUNCT3_CSRRW] = GET_SYS_HANDLER(csrrw),
        [SYSTEM_FUNCT3_CSRRS] = GET_SYS_HANDLER(csrrs),
        [SYSTEM_FUNCT3_CSRRC] = GET_SYS_HANDLER(csrrc),
        [SYSTEM_FUNCT3_CSRRWI] = GET_SYS_HANDLER(csrrwi),
        [SYSTEM_FUNCT3_CSRRSI] = GET_SYS_HANDLER(csrrsi),
        [SYSTEM_FUNCT3_CSRRCI] = GET_SYS_HANDLER(csrrci)
    };

    sys_ex_req_handler_t handler = sys_handlers[req->inst.i.funct3];
    if (handler) {
        handler(exu, req);
    } else {
        DBG_CHECK(0);
    }
}

void sys_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    if (ex_req->inst.base.opcode == OPCODE_SYSTEM) {
        CALL_SYS_HANDLER(sys_group, exu, ex_req);
    } else {
        DBG_CHECK(0);
    }
}
