#include "exu.h"
#include "dbg.h"

#define DECL_INST_HANDLER(inst) static void \
    inst##_handler(rv32i_t *s, const rv32i_inst_t *i, u32 *pc_offset)
#define DECL_GROUP_HANDLER(group) DECL_INST_HANDLER(group)
#define GET_HANDLER(inst) (&inst##_handler)
#define CALL_HANDLER(inst, s, i, o) inst##_handler(s, i, o)

typedef void (*inst_handler_t)(rv32i_t *s, const rv32i_inst_t *i, u32 *pc_offset);

static inline u32 sign_ext(u32 data, u32 width)
{
    if (data & (1u << (width - 1))) {
        data |= ((~0u) << width);
    }
    return data;
}

static inline u32 u_imm_decode(const rv32i_inst_t *i)
{
    return i->encoding.args.u_type.imm_31_12 << 12;
}

static inline u32 j_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->encoding.args.j_type.imm_10_1 << 1) |
              (i->encoding.args.j_type.imm_11 << 11) |
              (i->encoding.args.j_type.imm_19_12 << 12) |
              (i->encoding.args.j_type.imm_20 << 20);

    return sign_ext(src, 21);
}

static inline u32 i_imm_decode(const rv32i_inst_t *i)
{
    return sign_ext(i->encoding.args.i_type.imm_11_0, 12);
}

static inline u32 b_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->encoding.args.b_type.imm_4_1 << 1) |
              (i->encoding.args.b_type.imm_10_5 << 5) |
              (i->encoding.args.b_type.imm_11 << 11) |
              (i->encoding.args.b_type.imm_12 << 12);

    return sign_ext(src, 13);
}

static inline u32 s_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->encoding.args.s_type.imm_4_0) |
              (i->encoding.args.s_type.imm_11_5 << 5);

    return sign_ext(src, 12);
}

static inline u32 get_gpr(const rv32i_t *s, u32 i)
{
    return s->gpr[i];
}

static inline void set_gpr(rv32i_t *s, u32 i, u32 val)
{
    if (i > 0) {
        s->gpr[i] = val;
    }
}

DECL_INST_HANDLER(lui)
{
    u32 rd = i->encoding.args.u_type.rd;
    u32 imm = u_imm_decode(i);
    set_gpr(s, rd, imm);
}

DECL_INST_HANDLER(auipc)
{
    u32 rd = i->encoding.args.u_type.rd;
    u32 imm = u_imm_decode(i);
    set_gpr(s, rd, s->pc + imm);
}

DECL_INST_HANDLER(jal)
{
    u32 rd = i->encoding.args.j_type.rd;
    u32 imm = j_imm_decode(i);
    set_gpr(s, rd, s->pc + 4);
    *pc_offset = imm;
}

DECL_INST_HANDLER(jalr)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);

    set_gpr(s, rd, s->pc + 4);
    s->pc = get_gpr(s, rs1) + imm;
    *pc_offset = 0;
}

DECL_INST_HANDLER(beq)
{
    u32 rs1 = i->encoding.args.b_type.rs1;
    u32 rs2 = i->encoding.args.b_type.rs2;
    u32 imm = b_imm_decode(i);

    if (get_gpr(s, rs1) == get_gpr(s, rs2)) {
        *pc_offset = imm;
    }
}

DECL_INST_HANDLER(bne)
{
    u32 rs1 = i->encoding.args.b_type.rs1;
    u32 rs2 = i->encoding.args.b_type.rs2;
    u32 imm = b_imm_decode(i);

    if (get_gpr(s, rs1) != get_gpr(s, rs2)) {
        *pc_offset = imm;
    }
}

DECL_INST_HANDLER(blt)
{
    u32 rs1 = i->encoding.args.b_type.rs1;
    u32 rs2 = i->encoding.args.b_type.rs2;
    u32 imm = b_imm_decode(i);

    union { s32 s; u32 u; } n1, n2;
    n1.u = get_gpr(s, rs1);
    n2.u = get_gpr(s, rs2);

    if (n1.s < n2.s) {
        *pc_offset = imm;
    }
}

DECL_INST_HANDLER(bge)
{
    u32 rs1 = i->encoding.args.b_type.rs1;
    u32 rs2 = i->encoding.args.b_type.rs2;
    u32 imm = b_imm_decode(i);

    union { s32 s; u32 u; } n1, n2;
    n1.u = get_gpr(s, rs1);
    n2.u = get_gpr(s, rs2);

    if (n1.s >= n2.s) {
        *pc_offset = imm;
    }
}

DECL_INST_HANDLER(bltu)
{
    u32 rs1 = i->encoding.args.b_type.rs1;
    u32 rs2 = i->encoding.args.b_type.rs2;
    u32 imm = b_imm_decode(i);

    if (get_gpr(s, rs1) < get_gpr(s, rs2)) {
        *pc_offset = imm;
    }
}

DECL_INST_HANDLER(bgeu)
{
    u32 rs1 = i->encoding.args.b_type.rs1;
    u32 rs2 = i->encoding.args.b_type.rs2;
    u32 imm = b_imm_decode(i);

    if (get_gpr(s, rs1) >= get_gpr(s, rs2)) {
        *pc_offset = imm;
    }
}

DECL_GROUP_HANDLER(branch)
{
    static inst_handler_t branch_handlers[8] = {
        [BRANCH_FUNCT3_BEQ] = GET_HANDLER(beq),
        [BRANCH_FUNCT3_BNE] = GET_HANDLER(bne),
        [BRANCH_FUNCT3_BLT] = GET_HANDLER(blt),
        [BRANCH_FUNCT3_BGE] = GET_HANDLER(bge),
        [BRANCH_FUNCT3_BLTU] = GET_HANDLER(bltu),
        [BRANCH_FUNCT3_BGEU] = GET_HANDLER(bgeu)
    };

    inst_handler_t handler = branch_handlers[i->encoding.args.b_type.funct3];
    if (handler) {
        handler(s, i, pc_offset);
    } else {
        DBG_CHECK(0);
    }
}

DECL_INST_HANDLER(lb)
{
    DBG_CHECK(s->bus_if);

    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);

    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = get_gpr(s, rs1) + imm };
    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);

    set_gpr(s, rd, sign_ext(rsp.data & 0xff, 8));
}

DECL_INST_HANDLER(lh)
{
    DBG_CHECK(s->bus_if);

    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);

    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = get_gpr(s, rs1) + imm };
    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);

    set_gpr(s, rd, sign_ext(rsp.data & 0xffff, 16));
}

DECL_INST_HANDLER(lw)
{
    DBG_CHECK(s->bus_if);

    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);

    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = get_gpr(s, rs1) + imm };
    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);

    set_gpr(s, rd, rsp.data);
}

DECL_INST_HANDLER(lbu)
{
    DBG_CHECK(s->bus_if);

    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);

    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = get_gpr(s, rs1) + imm };
    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);

    set_gpr(s, rd, rsp.data & 0xff);
}

DECL_INST_HANDLER(lhu)
{
    DBG_CHECK(s->bus_if);

    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);

    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = get_gpr(s, rs1) + imm };
    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);

    set_gpr(s, rd, rsp.data & 0xffff);
}

DECL_GROUP_HANDLER(load)
{
    static inst_handler_t load_handlers[8] = {
        [LOAD_FUNCT3_LB] = GET_HANDLER(lb),
        [LOAD_FUNCT3_LH] = GET_HANDLER(lh),
        [LOAD_FUNCT3_LW] = GET_HANDLER(lw),
        [LOAD_FUNCT3_LBU] = GET_HANDLER(lbu),
        [LOAD_FUNCT3_LHU] = GET_HANDLER(lhu)
    };

    inst_handler_t handler = load_handlers[i->encoding.args.i_type.funct3];
    if (handler) {
        handler(s, i, pc_offset);
    } else {
        DBG_CHECK(0);
    }
}

DECL_INST_HANDLER(sb)
{
    DBG_CHECK(s->bus_if);

    u32 rs1 = i->encoding.args.s_type.rs1;
    u32 rs2 = i->encoding.args.s_type.rs2;
    u32 imm = s_imm_decode(i);

    bus_req_t req = {
        .cmd = BUS_CMD_WRITE,
        .addr = get_gpr(s, rs1) + imm,
        .data = get_gpr(s, rs2),
        .strobe = 0b0001
    };

    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);
}

DECL_INST_HANDLER(sh)
{
    DBG_CHECK(s->bus_if);

    u32 rs1 = i->encoding.args.s_type.rs1;
    u32 rs2 = i->encoding.args.s_type.rs2;
    u32 imm = s_imm_decode(i);

    bus_req_t req = {
        .cmd = BUS_CMD_WRITE,
        .addr = get_gpr(s, rs1) + imm,
        .data = get_gpr(s, rs2),
        .strobe = 0b0011
    };

    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);
}

DECL_INST_HANDLER(sw)
{
    DBG_CHECK(s->bus_if);

    u32 rs1 = i->encoding.args.s_type.rs1;
    u32 rs2 = i->encoding.args.s_type.rs2;
    u32 imm = s_imm_decode(i);

    bus_req_t req = {
        .cmd = BUS_CMD_WRITE,
        .addr = get_gpr(s, rs1) + imm,
        .data = get_gpr(s, rs2),
        .strobe = 0b1111
    };

    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);
    DBG_CHECK(rsp.ok);
}

DECL_GROUP_HANDLER(store)
{
    static inst_handler_t store_handlers[8] = {
        [STORE_FUNCT3_SB] = GET_HANDLER(sb),
        [STORE_FUNCT3_SH] = GET_HANDLER(sh),
        [STORE_FUNCT3_SW] = GET_HANDLER(sw)
    };

    inst_handler_t handler = store_handlers[i->encoding.args.s_type.funct3];
    if (handler) {
        handler(s, i, pc_offset);
    } else {
        DBG_CHECK(0);
    }
}

DECL_INST_HANDLER(addi)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);
    set_gpr(s, rd, get_gpr(s, rs1) + imm);
}

DECL_INST_HANDLER(slti)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);

    union { s32 s; u32 u; } n1, n2;
    n1.u = get_gpr(s, rs1);
    n2.u = imm;

    set_gpr(s, rd, n1.s < n2.s ? 1 : 0);
}

DECL_INST_HANDLER(sltiu)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);
    set_gpr(s, rd, get_gpr(s, rs1) < imm ? 1 : 0);
}

DECL_INST_HANDLER(xori)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);
    set_gpr(s, rd, get_gpr(s, rs1) ^ imm);
}

DECL_INST_HANDLER(ori)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);
    set_gpr(s, rd, get_gpr(s, rs1) | imm);
}

DECL_INST_HANDLER(andi)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i);
    set_gpr(s, rd, get_gpr(s, rs1) & imm);
}

DECL_INST_HANDLER(slli)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i) & 0b11111;
    set_gpr(s, rd, get_gpr(s, rs1) << imm);
}

DECL_INST_HANDLER(srli)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i) & 0b11111;
    set_gpr(s, rd, get_gpr(s, rs1) >> imm);
}

DECL_INST_HANDLER(srai)
{
    u32 rd = i->encoding.args.i_type.rd;
    u32 rs1 = i->encoding.args.i_type.rs1;
    u32 imm = i_imm_decode(i) & 0b11111;

    union { s32 s; u32 u; } ns, nd;
    ns.u = get_gpr(s, rs1);
    nd.s = ns.s >> imm;

    set_gpr(s, rd, nd.u);
}

DECL_GROUP_HANDLER(sri)
{
    if (i->encoding.args.i_type.imm_11_0 & 0b10000000000) {
        CALL_HANDLER(srai, s, i, pc_offset);
    } else {
        CALL_HANDLER(srli, s, i, pc_offset);
    }
}

DECL_GROUP_HANDLER(alu_imm)
{
    static inst_handler_t alu_imm_handlers[8] = {
        [ALU_IMM_FUNCT3_ADDI] = GET_HANDLER(addi),
        [ALU_IMM_FUNCT3_SLTI] = GET_HANDLER(slti),
        [ALU_IMM_FUNCT3_SLTIU] = GET_HANDLER(sltiu),
        [ALU_IMM_FUNCT3_XORI] = GET_HANDLER(xori),
        [ALU_IMM_FUNCT3_ORI] = GET_HANDLER(ori),
        [ALU_IMM_FUNCT3_ANDI] = GET_HANDLER(andi),
        [ALU_IMM_FUNCT3_SLI] = GET_HANDLER(slli),
        [ALU_IMM_FUNCT3_SRI] = GET_HANDLER(sri)
    };

    alu_imm_handlers[i->encoding.args.i_type.funct3](s, i, pc_offset);
}

DECL_INST_HANDLER(add)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;
    set_gpr(s, rd, get_gpr(s, rs1) + get_gpr(s, rs2));
}

DECL_INST_HANDLER(sub)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;
    set_gpr(s, rd, get_gpr(s, rs1) - get_gpr(s, rs2));
}

DECL_GROUP_HANDLER(add_sub)
{
    if (i->encoding.args.r_type.funct7 & 0b100000) {
        CALL_HANDLER(sub, s, i, pc_offset);
    } else {
        CALL_HANDLER(add, s, i, pc_offset);
    }
}

DECL_INST_HANDLER(sll)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;

    u32 s1 = get_gpr(s, rs1);
    u32 s2 = get_gpr(s, rs2) & 0b11111;
    set_gpr(s, rd, s1 << s2);
}

DECL_INST_HANDLER(slt)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;

    union { s32 s; u32 u; } s1, s2;
    s1.u = get_gpr(s, rs1);
    s2.u = get_gpr(s, rs2);

    set_gpr(s, rd, s1.s < s2.s ? 1 : 0);
}

DECL_INST_HANDLER(sltu)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;
    set_gpr(s, rd, get_gpr(s, rs1) < get_gpr(s, rs2) ? 1 : 0);
}

DECL_INST_HANDLER(xor)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;
    set_gpr(s, rd, get_gpr(s, rs1) ^ get_gpr(s, rs2));
}

DECL_INST_HANDLER(srl)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;

    u32 s1 = get_gpr(s, rs1);
    u32 s2 = get_gpr(s, rs2) & 0b11111;
    set_gpr(s, rd, s1 >> s2);
}

DECL_INST_HANDLER(sra)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;

    u32 s2 = get_gpr(s, rs2) & 0b11111;

    union { s32 s; u32 u; } s1, d;
    s1.u = get_gpr(s, rs1);
    d.s = s1.s >> s2;

    set_gpr(s, rd, d.u);
}

DECL_GROUP_HANDLER(sr)
{
    if (i->encoding.args.r_type.funct7 & 0b100000) {
        CALL_HANDLER(sra, s, i, pc_offset);
    } else {
        CALL_HANDLER(srl, s, i, pc_offset);
    }
}

DECL_INST_HANDLER(or)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;
    set_gpr(s, rd, get_gpr(s, rs1) | get_gpr(s, rs2));
}

DECL_INST_HANDLER(and)
{
    u32 rd = i->encoding.args.r_type.rd;
    u32 rs1 = i->encoding.args.r_type.rs1;
    u32 rs2 = i->encoding.args.r_type.rs2;
    set_gpr(s, rd, get_gpr(s, rs1) & get_gpr(s, rs2));
}

DECL_GROUP_HANDLER(alu)
{
    static inst_handler_t alu_handlers[8] = {
        [ALU_FUNCT3_ADD_SUB] = GET_HANDLER(add_sub),
        [ALU_FUNCT3_SL] = GET_HANDLER(sll),
        [ALU_FUNCT3_SLT] = GET_HANDLER(slt),
        [ALU_FUNCT3_SLTU] = GET_HANDLER(sltu),
        [ALU_FUNCT3_XOR] = GET_HANDLER(xor),
        [ALU_FUNCT3_SR] = GET_HANDLER(sr),
        [ALU_FUNCT3_OR] = GET_HANDLER(or),
        [ALU_FUNCT3_AND] = GET_HANDLER(and)
    };

    alu_handlers[i->encoding.args.i_type.funct3](s, i, pc_offset);
}

DECL_INST_HANDLER(fence)
{

}

DECL_INST_HANDLER(ecall)
{

}

DECL_INST_HANDLER(ebreak)
{

}

DECL_GROUP_HANDLER(sys)
{
    if (i->encoding.args.i_type.imm_11_0 == 0b000000000000) {
        CALL_HANDLER(ecall, s, i, pc_offset);
    } else if (i->encoding.args.i_type.imm_11_0 == 0b000000000001) {
        CALL_HANDLER(ebreak, s, i, pc_offset);
    } else {
        DBG_CHECK(0);
    }
}

void inst_handler(rv32i_t *s, const rv32i_inst_t *i, u32 *pc_offset)
{
    static inst_handler_t opcode_handlers[128] = {
        [OPCODE_LUI] = GET_HANDLER(lui),
        [OPCODE_AUIPC] = GET_HANDLER(auipc),
        [OPCODE_JAL] = GET_HANDLER(jal),
        [OPCODE_JALR] = GET_HANDLER(jalr),
        [OPCODE_BRANCH] = GET_HANDLER(branch),
        [OPCODE_LOAD] = GET_HANDLER(load),
        [OPCODE_STORE] = GET_HANDLER(store),
        [OPCODE_ALU_IMM] = GET_HANDLER(alu_imm),
        [OPCODE_ALU] = GET_HANDLER(alu),
        [OPCODE_MEM] = GET_HANDLER(fence),
        [OPCODE_SYSTEM] = GET_HANDLER(sys)
    };

    inst_handler_t handler = opcode_handlers[i->encoding.opcode];
    if (handler) {
        handler(s, i, pc_offset);
    } else {
        DBG_CHECK(0);
    }
}
