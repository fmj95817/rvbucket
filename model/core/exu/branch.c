#include "exu.h"
#include "utils.h"
#include "dbg.h"

#define DECL_BRANCH_HANDLER(inst) static void \
    inst##_ex_req_handler(exu_t *exu, const ex_req_if_t *req, ex_rsp_if_t *rsp)
#define GET_BRANCH_HANDLER(inst) &inst##_ex_req_handler
#define CALL_BRANCH_HANDLER(inst, exu, req, rsp) inst##_ex_req_handler(exu, req, rsp)

typedef void (*branch_ex_req_handler_t)(exu_t *exu, const ex_req_if_t *req, ex_rsp_if_t *rsp);

DECL_BRANCH_HANDLER(jal)
{
    u32 rd = req->inst.j.rd;
    i32 imm = j_imm_decode(&req->inst);
    set_gpr(exu, rd, req->pc + 4);
    rsp->taken = true;
    rsp->target_pc = req->pc + imm.u;

    DBG_LOG(LOG_TRACE, "jal %s, %d # 0x%08x\n",
        gpr_name(rd), imm.s, req->pc + imm.u);
}

DECL_BRANCH_HANDLER(jalr)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    i32 imm = i_imm_decode(&req->inst);

    set_gpr(exu, rd, req->pc + 4);
    rsp->taken = true;
    rsp->target_pc = get_gpr(exu, rs1) + imm.u;

    DBG_LOG(LOG_TRACE, "jalr %s, %s, %d # 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.s, rsp->target_pc);
}

DECL_BRANCH_HANDLER(beq)
{
    u32 rs1 = req->inst.b.rs1;
    u32 rs2 = req->inst.b.rs2;
    i32 imm = b_imm_decode(&req->inst);

    if (get_gpr(exu, rs1) == get_gpr(exu, rs2)) {
        rsp->taken = true;
        rsp->target_pc = req->pc + imm.u;
    }

    DBG_LOG(LOG_TRACE, "beq %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, req->pc + imm.u);
}

DECL_BRANCH_HANDLER(bne)
{
    u32 rs1 = req->inst.b.rs1;
    u32 rs2 = req->inst.b.rs2;
    i32 imm = b_imm_decode(&req->inst);

    if (get_gpr(exu, rs1) != get_gpr(exu, rs2)) {
        rsp->taken = true;
        rsp->target_pc = req->pc + imm.u;
    }

    DBG_LOG(LOG_TRACE, "bne %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, req->pc + imm.u);
}

DECL_BRANCH_HANDLER(blt)
{
    u32 rs1 = req->inst.b.rs1;
    u32 rs2 = req->inst.b.rs2;
    i32 imm = b_imm_decode(&req->inst);

    i32 n1, n2;
    n1.u = get_gpr(exu, rs1);
    n2.u = get_gpr(exu, rs2);

    if (n1.s < n2.s) {
        rsp->taken = true;
        rsp->target_pc = req->pc + imm.u;
    }

    DBG_LOG(LOG_TRACE, "blt %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, req->pc + imm.u);
}

DECL_BRANCH_HANDLER(bge)
{
    u32 rs1 = req->inst.b.rs1;
    u32 rs2 = req->inst.b.rs2;
    i32 imm = b_imm_decode(&req->inst);

    i32 n1, n2;
    n1.u = get_gpr(exu, rs1);
    n2.u = get_gpr(exu, rs2);

    if (n1.s >= n2.s) {
        rsp->taken = true;
        rsp->target_pc = req->pc + imm.u;
    }

    DBG_LOG(LOG_TRACE, "bge %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, req->pc + imm.u);
}

DECL_BRANCH_HANDLER(bltu)
{
    u32 rs1 = req->inst.b.rs1;
    u32 rs2 = req->inst.b.rs2;
    i32 imm = b_imm_decode(&req->inst);

    if (get_gpr(exu, rs1) < get_gpr(exu, rs2)) {
        rsp->taken = true;
        rsp->target_pc = req->pc + imm.u;
    }

    DBG_LOG(LOG_TRACE, "bltu %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, req->pc + imm.u);
}

DECL_BRANCH_HANDLER(bgeu)
{
    u32 rs1 = req->inst.b.rs1;
    u32 rs2 = req->inst.b.rs2;
    i32 imm = b_imm_decode(&req->inst);

    if (get_gpr(exu, rs1) >= get_gpr(exu, rs2)) {
        rsp->taken = true;
        rsp->target_pc = req->pc + imm.u;
    }

    DBG_LOG(LOG_TRACE, "bgeu %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, req->pc + imm.u);
}

DECL_BRANCH_HANDLER(branch_group)
{
    static branch_ex_req_handler_t branch_handlers[8] = {
        [BRANCH_FUNCT3_BEQ] = GET_BRANCH_HANDLER(beq),
        [BRANCH_FUNCT3_BNE] = GET_BRANCH_HANDLER(bne),
        [BRANCH_FUNCT3_BLT] = GET_BRANCH_HANDLER(blt),
        [BRANCH_FUNCT3_BGE] = GET_BRANCH_HANDLER(bge),
        [BRANCH_FUNCT3_BLTU] = GET_BRANCH_HANDLER(bltu),
        [BRANCH_FUNCT3_BGEU] = GET_BRANCH_HANDLER(bgeu)
    };

    branch_ex_req_handler_t handler = branch_handlers[req->inst.b.funct3];
    if (handler) {
        handler(exu, req, rsp);
    } else {
        DBG_CHECK(0);
    }
}

void branch_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    ex_rsp_if_t ex_rsp;
    ex_rsp.taken = false;

    if (ex_req->inst.base.opcode == OPCODE_JAL) {
        CALL_BRANCH_HANDLER(jal, exu, ex_req, &ex_rsp);
    } else if (ex_req->inst.base.opcode == OPCODE_JALR) {
        CALL_BRANCH_HANDLER(jalr, exu, ex_req, &ex_rsp);
    } else if (ex_req->inst.base.opcode == OPCODE_BRANCH) {
        CALL_BRANCH_HANDLER(branch_group, exu, ex_req, &ex_rsp);
    } else {
        DBG_CHECK(0);
    }

    itf_write(exu->ex_rsp_mst, &ex_rsp);
}
