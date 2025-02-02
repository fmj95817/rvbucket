#include "exu.h"
#include "utils.h"
#include "dbg.h"

#define DECL_LDST_IEX_REQ_HANDLER(inst) static void inst##_ex_req_handler( \
    exu_t *exu, const ex_req_if_t *ex_req, ldst_req_if_t *ldst_req)
#define DECL_LDST_LSU_RSP_HANDLER(inst) static void inst##_lsu_rsp_handler( \
    exu_t *exu, const ldst_rsp_if_t *ldst_rsp)

#define GET_LDST_IEX_REQ_HANDLER(inst) &inst##_ex_req_handler
#define GET_LDST_LSU_RSP_HANDLER(inst) &inst##_lsu_rsp_handler

#define CALL_LDST_IEX_REQ_HANDLER(inst, exu, ex_req, ldst_req) \
    inst##_ex_req_handler(exu, ex_req, ldst_req)
#define CALL_LDST_LSU_RSP_HANDLER(inst, exu, ldst_rsp) \
    inst##_lsu_rsp_handler(exu, ldst_rsp)

typedef void (*ldst_ex_req_handler_t)( \
    exu_t *exu, const ex_req_if_t *ex_req, ldst_req_if_t *ldst_req);
typedef void (*ldst_lsu_rsp_handler_t)( \
    exu_t *exu, const ldst_rsp_if_t *ldst_rsp);

DECL_LDST_IEX_REQ_HANDLER(lb)
{
    u32 rd = ex_req->inst.i.rd;
    u32 rs1 = ex_req->inst.i.rs1;
    i32 imm = i_imm_decode(&ex_req->inst);

    ldst_req->st = false;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;

    DBG_LOG(LOG_TRACE, "lb %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_LSU_RSP_HANDLER(lb)
{
    set_gpr(exu, exu->ld_rd, sign_ext(ldst_rsp->data & 0xff, 8));
}

DECL_LDST_IEX_REQ_HANDLER(lh)
{
    u32 rd = ex_req->inst.i.rd;
    u32 rs1 = ex_req->inst.i.rs1;
    i32 imm = i_imm_decode(&ex_req->inst);

    ldst_req->st = false;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;

    DBG_LOG(LOG_TRACE, "lh %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_LSU_RSP_HANDLER(lh)
{
    set_gpr(exu, exu->ld_rd, sign_ext(ldst_rsp->data & 0xffff, 16));
}

DECL_LDST_IEX_REQ_HANDLER(lw)
{
    u32 rd = ex_req->inst.i.rd;
    u32 rs1 = ex_req->inst.i.rs1;
    i32 imm = i_imm_decode(&ex_req->inst);

    ldst_req->st = false;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;

    DBG_LOG(LOG_TRACE, "lw %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_LSU_RSP_HANDLER(lw)
{
    set_gpr(exu, exu->ld_rd, ldst_rsp->data);
}

DECL_LDST_IEX_REQ_HANDLER(lbu)
{
    u32 rd = ex_req->inst.i.rd;
    u32 rs1 = ex_req->inst.i.rs1;
    i32 imm = i_imm_decode(&ex_req->inst);

    ldst_req->st = false;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;

    DBG_LOG(LOG_TRACE, "lbu %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_LSU_RSP_HANDLER(lbu)
{
    set_gpr(exu, exu->ld_rd, ldst_rsp->data & 0xff);
}

DECL_LDST_IEX_REQ_HANDLER(lhu)
{
    u32 rd = ex_req->inst.i.rd;
    u32 rs1 = ex_req->inst.i.rs1;
    i32 imm = i_imm_decode(&ex_req->inst);

    ldst_req->st = false;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;

    DBG_LOG(LOG_TRACE, "lbu %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_LSU_RSP_HANDLER(lhu)
{
    set_gpr(exu, exu->ld_rd, ldst_rsp->data & 0xffff);
}

DECL_LDST_IEX_REQ_HANDLER(load_group)
{
    static ldst_ex_req_handler_t handlers[8] = {
        [LOAD_FUNCT3_LB] = GET_LDST_IEX_REQ_HANDLER(lb),
        [LOAD_FUNCT3_LH] = GET_LDST_IEX_REQ_HANDLER(lh),
        [LOAD_FUNCT3_LW] = GET_LDST_IEX_REQ_HANDLER(lw),
        [LOAD_FUNCT3_LBU] = GET_LDST_IEX_REQ_HANDLER(lbu),
        [LOAD_FUNCT3_LHU] = GET_LDST_IEX_REQ_HANDLER(lhu)
    };

    ldst_ex_req_handler_t handler = handlers[ex_req->inst.i.funct3];
    if (handler) {
        handler(exu, ex_req, ldst_req);
    } else {
        DBG_CHECK(0);
    }
}

DECL_LDST_LSU_RSP_HANDLER(load_group)
{
    static ldst_lsu_rsp_handler_t handlers[8] = {
        [LOAD_FUNCT3_LB] = GET_LDST_LSU_RSP_HANDLER(lb),
        [LOAD_FUNCT3_LH] = GET_LDST_LSU_RSP_HANDLER(lh),
        [LOAD_FUNCT3_LW] = GET_LDST_LSU_RSP_HANDLER(lw),
        [LOAD_FUNCT3_LBU] = GET_LDST_LSU_RSP_HANDLER(lbu),
        [LOAD_FUNCT3_LHU] = GET_LDST_LSU_RSP_HANDLER(lhu)
    };

    ldst_lsu_rsp_handler_t handler = handlers[exu->ld_funct3];
    if (handler) {
        handler(exu, ldst_rsp);
    } else {
        DBG_CHECK(0);
    }
}

DECL_LDST_IEX_REQ_HANDLER(sb)
{
    u32 rs1 = ex_req->inst.s.rs1;
    u32 rs2 = ex_req->inst.s.rs2;
    i32 imm = s_imm_decode(&ex_req->inst);

    ldst_req->st = true;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;
    ldst_req->data = get_gpr(exu, rs2);
    ldst_req->strobe = 0b0001;

    DBG_LOG(LOG_TRACE, "sb %s, %d(%s) # 0x%08x\n",
        gpr_name(rs2), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_IEX_REQ_HANDLER(sh)
{
    u32 rs1 = ex_req->inst.s.rs1;
    u32 rs2 = ex_req->inst.s.rs2;
    i32 imm = s_imm_decode(&ex_req->inst);

    ldst_req->st = true;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;
    ldst_req->data = get_gpr(exu, rs2);
    ldst_req->strobe = 0b0011;

    DBG_LOG(LOG_TRACE, "sh %s, %d(%s) # 0x%08x\n",
        gpr_name(rs2), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_IEX_REQ_HANDLER(sw)
{
    u32 rs1 = ex_req->inst.s.rs1;
    u32 rs2 = ex_req->inst.s.rs2;
    i32 imm = s_imm_decode(&ex_req->inst);

    ldst_req->st = true;
    ldst_req->addr = get_gpr(exu, rs1) + imm.u;
    ldst_req->data = get_gpr(exu, rs2);
    ldst_req->strobe = 0b1111;

    DBG_LOG(LOG_TRACE, "sw %s, %d(%s) # 0x%08x\n",
        gpr_name(rs2), imm.s, gpr_name(rs1), ldst_req->addr);
}

DECL_LDST_IEX_REQ_HANDLER(store_group)
{
    static ldst_ex_req_handler_t handlers[8] = {
        [STORE_FUNCT3_SB] = GET_LDST_IEX_REQ_HANDLER(sb),
        [STORE_FUNCT3_SH] = GET_LDST_IEX_REQ_HANDLER(sh),
        [STORE_FUNCT3_SW] = GET_LDST_IEX_REQ_HANDLER(sw)
    };

    ldst_ex_req_handler_t handler = handlers[ex_req->inst.s.funct3];
    if (handler) {
        handler(exu, ex_req, ldst_req);
    } else {
        DBG_CHECK(0);
    }
}

void ldst_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    if (itf_fifo_full(exu->ldst_req_mst)) {
        return;
    }

    if (exu->ldst_req_pend) {
        return;
    }

    ldst_req_if_t ldst_req;
    if (ex_req->inst.base.opcode == OPCODE_LOAD) {
        CALL_LDST_IEX_REQ_HANDLER(load_group, exu, ex_req, &ldst_req);
    } else if (ex_req->inst.base.opcode == OPCODE_STORE) {
        CALL_LDST_IEX_REQ_HANDLER(store_group, exu, ex_req, &ldst_req);
    } else {
        DBG_CHECK(0);
    }

    itf_write(exu->ldst_req_mst, &ldst_req);
    exu->ldst_req_pend = true;
    exu->ldst_opcode = ex_req->inst.base.opcode;

    if (ex_req->inst.base.opcode == OPCODE_LOAD) {
        exu->ld_rd = ex_req->inst.i.rd;
        exu->ld_funct3 = ex_req->inst.i.funct3;
    }
}

void ldst_lsu_rsp_proc(exu_t *exu, const ldst_rsp_if_t *ldst_rsp)
{
    if (exu->ldst_opcode == OPCODE_LOAD) {
        CALL_LDST_LSU_RSP_HANDLER(load_group, exu, ldst_rsp);
    }
    exu->ldst_req_pend = false;
}
