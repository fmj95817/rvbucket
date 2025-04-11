#include "exu.h"
#include "utils.h"
#include "dbg/chk.h"
#include "dbg/log.h"

#define DECL_ALU_HANDLER(inst) static void inst##_ex_req_handler(exu_t *exu, const ex_req_if_t *req)
#define GET_ALU_HANDLER(inst) &inst##_ex_req_handler
#define CALL_ALU_HANDLER(inst, exu, req) inst##_ex_req_handler(exu, req)

typedef void (*alu_ex_req_handler_t)(exu_t *exu, const ex_req_if_t *req);

DECL_ALU_HANDLER(addi)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    i32 imm = i_imm_decode(&req->inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) + imm.u);

    DBG_LOG(LOG_TRACE, "addi %s, %s, %d\n",
        gpr_name(rd), gpr_name(rs1), imm.s);
}

DECL_ALU_HANDLER(slti)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    i32 imm = i_imm_decode(&req->inst);

    i32 n1, n2;
    n1.u = get_gpr(exu, rs1);
    n2.u = imm.u;

    set_gpr(exu, rd, n1.s < n2.s ? 1 : 0);

    DBG_LOG(LOG_TRACE, "slti %s, %s, %d\n",
        gpr_name(rd), gpr_name(rs1), imm.s);
}

DECL_ALU_HANDLER(sltiu)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    i32 imm = i_imm_decode(&req->inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) < imm.u ? 1 : 0);

    DBG_LOG(LOG_TRACE, "sltiu %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_ALU_HANDLER(xori)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    i32 imm = i_imm_decode(&req->inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) ^ imm.u);

    DBG_LOG(LOG_TRACE, "xori %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_ALU_HANDLER(ori)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    i32 imm = i_imm_decode(&req->inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) | imm.u);

    DBG_LOG(LOG_TRACE, "ori %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_ALU_HANDLER(andi)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    i32 imm = i_imm_decode(&req->inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) & imm.u);

    DBG_LOG(LOG_TRACE, "andi %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_ALU_HANDLER(slli)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    u32 imm = i_imm_decode(&req->inst).u & 0b11111;
    set_gpr(exu, rd, get_gpr(exu, rs1) << imm);

    DBG_LOG(LOG_TRACE, "slli %s, %s, %u\n",
        gpr_name(rd), gpr_name(rs1), imm);
}

DECL_ALU_HANDLER(srli)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    u32 imm = i_imm_decode(&req->inst).u & 0b11111;
    set_gpr(exu, rd, get_gpr(exu, rs1) >> imm);

    DBG_LOG(LOG_TRACE, "srli %s, %s, %u\n",
        gpr_name(rd), gpr_name(rs1), imm);
}

DECL_ALU_HANDLER(srai)
{
    u32 rd = req->inst.i.rd;
    u32 rs1 = req->inst.i.rs1;
    u32 imm = i_imm_decode(&req->inst).u & 0b11111;

    i32 ns, nd;
    ns.u = get_gpr(exu, rs1);
    nd.s = ns.s >> imm;

    set_gpr(exu, rd, nd.u);

    DBG_LOG(LOG_TRACE, "srai %s, %s, %u\n",
        gpr_name(rd), gpr_name(rs1), imm);
}

DECL_ALU_HANDLER(sri)
{
    if (req->inst.i.imm_11_0 & 0b10000000000) {
        CALL_ALU_HANDLER(srai, exu, req);
    } else {
        CALL_ALU_HANDLER(srli, exu, req);
    }
}

DECL_ALU_HANDLER(alui_group)
{
    static alu_ex_req_handler_t alui_handlers[8] = {
        [ALUI_FUNCT3_ADDI] = GET_ALU_HANDLER(addi),
        [ALUI_FUNCT3_SLTI] = GET_ALU_HANDLER(slti),
        [ALUI_FUNCT3_SLTIU] = GET_ALU_HANDLER(sltiu),
        [ALUI_FUNCT3_XORI] = GET_ALU_HANDLER(xori),
        [ALUI_FUNCT3_ORI] = GET_ALU_HANDLER(ori),
        [ALUI_FUNCT3_ANDI] = GET_ALU_HANDLER(andi),
        [ALUI_FUNCT3_SLI] = GET_ALU_HANDLER(slli),
        [ALUI_FUNCT3_SRI] = GET_ALU_HANDLER(sri)
    };

    alui_handlers[req->inst.i.funct3](exu, req);
}

DECL_ALU_HANDLER(add)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) + get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "add %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(sub)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) - get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "sub %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(add_sub)
{
    if (req->inst.r.funct7 & 0b100000) {
        CALL_ALU_HANDLER(sub, exu, req);
    } else {
        CALL_ALU_HANDLER(add, exu, req);
    }
}

DECL_ALU_HANDLER(sll)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;

    u32 s1 = get_gpr(exu, rs1);
    u32 s2 = get_gpr(exu, rs2) & 0b11111;
    set_gpr(exu, rd, s1 << s2);

    DBG_LOG(LOG_TRACE, "sll %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(slt)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;

    i32 s1, s2;
    s1.u = get_gpr(exu, rs1);
    s2.u = get_gpr(exu, rs2);

    set_gpr(exu, rd, s1.s < s2.s ? 1 : 0);

    DBG_LOG(LOG_TRACE, "slt %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(sltu)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) < get_gpr(exu, rs2) ? 1 : 0);

    DBG_LOG(LOG_TRACE, "sltu %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(xor)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) ^ get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "xor %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(srl)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;

    u32 s1 = get_gpr(exu, rs1);
    u32 s2 = get_gpr(exu, rs2) & 0b11111;
    set_gpr(exu, rd, s1 >> s2);

    DBG_LOG(LOG_TRACE, "srl %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(sra)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;

    u32 s2 = get_gpr(exu, rs2) & 0b11111;

    i32 s1, d;
    s1.u = get_gpr(exu, rs1);
    d.s = s1.s >> s2;

    set_gpr(exu, rd, d.u);

    DBG_LOG(LOG_TRACE, "sra %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(sr)
{
    if (req->inst.r.funct7 & 0b100000) {
        CALL_ALU_HANDLER(sra, exu, req);
    } else {
        CALL_ALU_HANDLER(srl, exu, req);
    }
}

DECL_ALU_HANDLER(or)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) | get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "or %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(and)
{
    u32 rd = req->inst.r.rd;
    u32 rs1 = req->inst.r.rs1;
    u32 rs2 = req->inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) & get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "and %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_ALU_HANDLER(alu_group)
{
    static alu_ex_req_handler_t alu_handlers[8] = {
        [ALU_FUNCT3_ADD_SUB] = GET_ALU_HANDLER(add_sub),
        [ALU_FUNCT3_SL] = GET_ALU_HANDLER(sll),
        [ALU_FUNCT3_SLT] = GET_ALU_HANDLER(slt),
        [ALU_FUNCT3_SLTU] = GET_ALU_HANDLER(sltu),
        [ALU_FUNCT3_XOR] = GET_ALU_HANDLER(xor),
        [ALU_FUNCT3_SR] = GET_ALU_HANDLER(sr),
        [ALU_FUNCT3_OR] = GET_ALU_HANDLER(or),
        [ALU_FUNCT3_AND] = GET_ALU_HANDLER(and)
    };

    alu_handlers[req->inst.r.funct3](exu, req);
}

void alu_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    if (ex_req->inst.base.opcode == OPCODE_ALUI) {
        CALL_ALU_HANDLER(alui_group, exu, ex_req);
    } else if (ex_req->inst.base.opcode == OPCODE_ALU) {
        CALL_ALU_HANDLER(alu_group, exu, ex_req);
    } else {
        DBG_CHECK(0);
    }
}