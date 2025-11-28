#include "exu.h"
#include "utils.h"
#include "spec/csr.h"
#include "dbg/chk.h"
#include "dbg/log.h"

#define DECL_SYS_HANDLER(inst) static void inst##_ex_req_handler(exu_t *exu, const ex_req_if_t *req)
#define CALL_SYS_HANDLER(inst, exu, req) inst##_ex_req_handler(exu, req)
#define GET_SYS_HANDLER(inst) &inst##_ex_req_handler

typedef void (*sys_ex_req_handler_t)(exu_t *exu, const ex_req_if_t *req);

DECL_SYS_HANDLER(ecall)
{
    DBG_LOG(LOG_TRACE, "ecall\n");
}

DECL_SYS_HANDLER(ebreak)
{
    DBG_LOG(LOG_TRACE, "ebreak\n");
}

DECL_SYS_HANDLER(sret)
{
    DBG_LOG(LOG_TRACE, "sret\n");
}

DECL_SYS_HANDLER(mret)
{
    DBG_LOG(LOG_TRACE, "mret\n");
}

DECL_SYS_HANDLER(wfi)
{
    DBG_LOG(LOG_TRACE, "wfi\n");
}

DECL_SYS_HANDLER(sfence_vma)
{
    DBG_LOG(LOG_TRACE, "sfence.vma\n");
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

DECL_SYS_HANDLER(csrrw)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 new_val = get_gpr(exu, req->inst.i.rs1);
    DBG_CHECK(csr_write(exu, req->inst.i.imm_11_0, new_val));

    DBG_LOG(LOG_TRACE, "csrrw, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(req->inst.i.imm_11_0), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrs)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = get_gpr(exu, req->inst.i.rs1);
    if (mask != 0) {
        DBG_CHECK(csr_write(exu, req->inst.i.imm_11_0, old_val | mask));
    }
    DBG_LOG(LOG_TRACE, "csrrs, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(req->inst.i.imm_11_0), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrc)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = get_gpr(exu, req->inst.i.rs1);
    if (mask != 0) {
        DBG_CHECK(csr_write(exu, req->inst.i.imm_11_0, old_val & (~mask)));
    }
    DBG_LOG(LOG_TRACE, "csrrc, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(req->inst.i.imm_11_0), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrwi)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 new_val = req->inst.i.rs1;
    DBG_CHECK(csr_write(exu, req->inst.i.imm_11_0, new_val));

    DBG_LOG(LOG_TRACE, "csrrwi, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(req->inst.i.imm_11_0), req->inst.i.rs1);
}

DECL_SYS_HANDLER(csrrsi)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = req->inst.i.rs1;
    if (mask != 0) {
        DBG_CHECK(csr_write(exu, req->inst.i.imm_11_0, old_val | mask));
    }
    DBG_LOG(LOG_TRACE, "csrrsi, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(req->inst.i.imm_11_0), req->inst.i.rs1);
}

DECL_SYS_HANDLER(csrrci)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = req->inst.i.rs1;
    if (mask != 0) {
        DBG_CHECK(csr_write(exu, req->inst.i.imm_11_0, old_val & (~mask)));
    }
    DBG_LOG(LOG_TRACE, "csrrci, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        rv32g_csr_name(req->inst.i.imm_11_0), req->inst.i.rs1);
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