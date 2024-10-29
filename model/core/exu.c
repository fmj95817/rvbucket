#include "exu.h"
#include "lsu.h"
#include "dbg.h"

#define DECL_INST_HANDLER(inst) static void inst##_handler(exu_t *exu, iexec_if_t *i)
#define DECL_GROUP_HANDLER(group) DECL_INST_HANDLER(group)
#define GET_HANDLER(inst) (&inst##_handler)
#define CALL_HANDLER(inst, exu, i) inst##_handler(exu, i)

typedef void (*inst_handler_t)(exu_t *exu, iexec_if_t *i);

static inline u32 sign_ext(u32 data, u32 width)
{
    if (data & (1u << (width - 1))) {
        data |= ((~0u) << width);
    }
    return data;
}

static inline u32 u_imm_decode(const rv32i_inst_t *i)
{
    return i->u.imm_31_12 << 12;
}

static inline i32 j_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->j.imm_10_1 << 1) |
              (i->j.imm_11 << 11) |
              (i->j.imm_19_12 << 12) |
              (i->j.imm_20 << 20);

    i32 dst = { .u = sign_ext(src, 21) };
    return dst;
}

static inline i32 i_imm_decode(const rv32i_inst_t *i)
{
    i32 dst = { .u = sign_ext(i->i.imm_11_0, 12) };
    return dst;
}

static inline i32 b_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->b.imm_4_1 << 1) |
              (i->b.imm_10_5 << 5) |
              (i->b.imm_11 << 11) |
              (i->b.imm_12 << 12);

    i32 dst = { .u = sign_ext(src, 13) };
    return dst;
}

static inline i32 s_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->s.imm_4_0) |
              (i->s.imm_11_5 << 5);

    i32 dst = { .u = sign_ext(src, 12) };
    return dst;
}

static inline u32 get_gpr(const exu_t *exu, u32 i)
{
    return exu->gpr[i];
}

static inline void set_gpr(exu_t *exu, u32 i, u32 val)
{
    if (i > 0) {
        exu->gpr[i] = val;
    }
}

static inline const char *gpr_name(u32 i)
{
    DBG_CHECK(i < 32);

    static const char *names[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };

    return names[i];
}

DECL_INST_HANDLER(lui)
{
    u32 rd = i->req.inst.u.rd;
    u32 imm = u_imm_decode(&i->req.inst);
    set_gpr(exu, rd, imm);

    DBG_LOG(LOG_TRACE, "lui %s, 0x%08x\n", gpr_name(rd), imm);
}

DECL_INST_HANDLER(auipc)
{
    u32 rd = i->req.inst.u.rd;
    u32 imm = u_imm_decode(&i->req.inst);
    set_gpr(exu, rd, i->req.pc + imm);

    DBG_LOG(LOG_TRACE, "auipc %s, 0x%08x\n", gpr_name(rd), imm);
}

DECL_INST_HANDLER(jal)
{
    u32 rd = i->req.inst.j.rd;
    i32 imm = j_imm_decode(&i->req.inst);
    set_gpr(exu, rd, i->req.pc + 4);
    i->rsp.taken = true;
    i->rsp.pc_offset = imm.u;

    DBG_LOG(LOG_TRACE, "jal %s, %d # 0x%08x\n",
        gpr_name(rd), imm.s, i->req.pc + imm.u);
}

DECL_INST_HANDLER(jalr)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);

    set_gpr(exu, rd, i->req.pc + 4);
    i->rsp.taken = true;
    i->rsp.abs_jump = true;
    i->rsp.target_pc = get_gpr(exu, rs1) + imm.u;

    DBG_LOG(LOG_TRACE, "jalr %s, %s, %d # 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.s, i->rsp.target_pc);
}

DECL_INST_HANDLER(beq)
{
    u32 rs1 = i->req.inst.b.rs1;
    u32 rs2 = i->req.inst.b.rs2;
    i32 imm = b_imm_decode(&i->req.inst);

    if (get_gpr(exu, rs1) == get_gpr(exu, rs2)) {
        i->rsp.taken = true;
        i->rsp.pc_offset = imm.u;
    }

    DBG_LOG(LOG_TRACE, "beq %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, i->req.pc + imm.u);
}

DECL_INST_HANDLER(bne)
{
    u32 rs1 = i->req.inst.b.rs1;
    u32 rs2 = i->req.inst.b.rs2;
    i32 imm = b_imm_decode(&i->req.inst);

    if (get_gpr(exu, rs1) != get_gpr(exu, rs2)) {
        i->rsp.taken = true;
        i->rsp.pc_offset = imm.u;
    }

    DBG_LOG(LOG_TRACE, "bne %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, i->req.pc + imm.u);
}

DECL_INST_HANDLER(blt)
{
    u32 rs1 = i->req.inst.b.rs1;
    u32 rs2 = i->req.inst.b.rs2;
    i32 imm = b_imm_decode(&i->req.inst);

    i32 n1, n2;
    n1.u = get_gpr(exu, rs1);
    n2.u = get_gpr(exu, rs2);

    if (n1.s < n2.s) {
        i->rsp.taken = true;
        i->rsp.pc_offset = imm.u;
    }

    DBG_LOG(LOG_TRACE, "blt %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, i->req.pc + imm.u);
}

DECL_INST_HANDLER(bge)
{
    u32 rs1 = i->req.inst.b.rs1;
    u32 rs2 = i->req.inst.b.rs2;
    i32 imm = b_imm_decode(&i->req.inst);

    i32 n1, n2;
    n1.u = get_gpr(exu, rs1);
    n2.u = get_gpr(exu, rs2);

    if (n1.s >= n2.s) {
        i->rsp.taken = true;
        i->rsp.pc_offset = imm.u;
    }

    DBG_LOG(LOG_TRACE, "bge %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, i->req.pc + imm.u);
}

DECL_INST_HANDLER(bltu)
{
    u32 rs1 = i->req.inst.b.rs1;
    u32 rs2 = i->req.inst.b.rs2;
    i32 imm = b_imm_decode(&i->req.inst);

    if (get_gpr(exu, rs1) < get_gpr(exu, rs2)) {
        i->rsp.taken = true;
        i->rsp.pc_offset = imm.u;
    }

    DBG_LOG(LOG_TRACE, "bltu %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, i->req.pc + imm.u);
}

DECL_INST_HANDLER(bgeu)
{
    u32 rs1 = i->req.inst.b.rs1;
    u32 rs2 = i->req.inst.b.rs2;
    i32 imm = b_imm_decode(&i->req.inst);

    if (get_gpr(exu, rs1) >= get_gpr(exu, rs2)) {
        i->rsp.taken = true;
        i->rsp.pc_offset = imm.u;
    }

    DBG_LOG(LOG_TRACE, "bgeu %s, %s, %d # 0x%08x\n",
        gpr_name(rs1), gpr_name(rs2), imm.s, i->req.pc + imm.u);
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

    inst_handler_t handler = branch_handlers[i->req.inst.b.funct3];
    if (handler) {
        handler(exu, i);
    } else {
        DBG_CHECK(0);
    }
}

DECL_INST_HANDLER(lb)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = false;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;

    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    set_gpr(exu, rd, sign_ext(ldst_if.rsp.data & 0xff, 8));

    DBG_LOG(LOG_TRACE, "lb %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_if.req.addr);
}

DECL_INST_HANDLER(lh)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = false;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;

    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    set_gpr(exu, rd, sign_ext(ldst_if.rsp.data & 0xffff, 16));

    DBG_LOG(LOG_TRACE, "lh %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_if.req.addr);
}

DECL_INST_HANDLER(lw)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = false;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;

    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    set_gpr(exu, rd, ldst_if.rsp.data);

    DBG_LOG(LOG_TRACE, "lw %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_if.req.addr);
}

DECL_INST_HANDLER(lbu)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = false;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;

    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    set_gpr(exu, rd, ldst_if.rsp.data & 0xff);

    DBG_LOG(LOG_TRACE, "lbu %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_if.req.addr);
}

DECL_INST_HANDLER(lhu)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = false;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;
    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    set_gpr(exu, rd, ldst_if.rsp.data & 0xffff);

    DBG_LOG(LOG_TRACE, "lbu %s, %d(%s) # 0x%08x\n",
        gpr_name(rd), imm.s, gpr_name(rs1), ldst_if.req.addr);
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

    inst_handler_t handler = load_handlers[i->req.inst.i.funct3];
    if (handler) {
        handler(exu, i);
    } else {
        DBG_CHECK(0);
    }
}

DECL_INST_HANDLER(sb)
{
    u32 rs1 = i->req.inst.s.rs1;
    u32 rs2 = i->req.inst.s.rs2;
    i32 imm = s_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = true;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;
    ldst_if.req.data = get_gpr(exu, rs2);
    ldst_if.req.strobe = 0b0001;

    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    DBG_LOG(LOG_TRACE, "sb %s, %d(%s) # 0x%08x\n",
        gpr_name(rs2), imm.s, gpr_name(rs1), ldst_if.req.addr);
}

DECL_INST_HANDLER(sh)
{
    u32 rs1 = i->req.inst.s.rs1;
    u32 rs2 = i->req.inst.s.rs2;
    i32 imm = s_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = true;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;
    ldst_if.req.data = get_gpr(exu, rs2);
    ldst_if.req.strobe = 0b0011;

    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    DBG_LOG(LOG_TRACE, "sh %s, %d(%s) # 0x%08x\n",
        gpr_name(rs2), imm.s, gpr_name(rs1), ldst_if.req.addr);
}

DECL_INST_HANDLER(sw)
{
    u32 rs1 = i->req.inst.s.rs1;
    u32 rs2 = i->req.inst.s.rs2;
    i32 imm = s_imm_decode(&i->req.inst);

    ldst_if_t ldst_if;
    ldst_if.req.st = true;
    ldst_if.req.addr = get_gpr(exu, rs1) + imm.u;
    ldst_if.req.data = get_gpr(exu, rs2);
    ldst_if.req.strobe = 0b1111;

    lsu_ldst_trans_handler(exu->lsu, &ldst_if);
    DBG_CHECK(ldst_if.rsp.ok);

    DBG_LOG(LOG_TRACE, "sw %s, %d(%s) # 0x%08x\n",
        gpr_name(rs2), imm.s, gpr_name(rs1), ldst_if.req.addr);
}

DECL_GROUP_HANDLER(store)
{
    static inst_handler_t store_handlers[8] = {
        [STORE_FUNCT3_SB] = GET_HANDLER(sb),
        [STORE_FUNCT3_SH] = GET_HANDLER(sh),
        [STORE_FUNCT3_SW] = GET_HANDLER(sw)
    };

    inst_handler_t handler = store_handlers[i->req.inst.s.funct3];
    if (handler) {
        handler(exu, i);
    } else {
        DBG_CHECK(0);
    }
}

DECL_INST_HANDLER(addi)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) + imm.u);

    DBG_LOG(LOG_TRACE, "addi %s, %s, %d\n",
        gpr_name(rd), gpr_name(rs1), imm.s);
}

DECL_INST_HANDLER(slti)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);

    i32 n1, n2;
    n1.u = get_gpr(exu, rs1);
    n2.u = imm.u;

    set_gpr(exu, rd, n1.s < n2.s ? 1 : 0);

    DBG_LOG(LOG_TRACE, "slti %s, %s, %d\n",
        gpr_name(rd), gpr_name(rs1), imm.s);
}

DECL_INST_HANDLER(sltiu)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) < imm.u ? 1 : 0);

    DBG_LOG(LOG_TRACE, "sltiu %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_INST_HANDLER(xori)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) ^ imm.u);

    DBG_LOG(LOG_TRACE, "xori %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_INST_HANDLER(ori)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) | imm.u);

    DBG_LOG(LOG_TRACE, "ori %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_INST_HANDLER(andi)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    i32 imm = i_imm_decode(&i->req.inst);
    set_gpr(exu, rd, get_gpr(exu, rs1) & imm.u);

    DBG_LOG(LOG_TRACE, "andi %s, %s, 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), imm.u);
}

DECL_INST_HANDLER(slli)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    u32 imm = i_imm_decode(&i->req.inst).u & 0b11111;
    set_gpr(exu, rd, get_gpr(exu, rs1) << imm);

    DBG_LOG(LOG_TRACE, "slli %s, %s, %u\n",
        gpr_name(rd), gpr_name(rs1), imm);
}

DECL_INST_HANDLER(srli)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    u32 imm = i_imm_decode(&i->req.inst).u & 0b11111;
    set_gpr(exu, rd, get_gpr(exu, rs1) >> imm);

    DBG_LOG(LOG_TRACE, "srli %s, %s, %u\n",
        gpr_name(rd), gpr_name(rs1), imm);
}

DECL_INST_HANDLER(srai)
{
    u32 rd = i->req.inst.i.rd;
    u32 rs1 = i->req.inst.i.rs1;
    u32 imm = i_imm_decode(&i->req.inst).u & 0b11111;

    i32 ns, nd;
    ns.u = get_gpr(exu, rs1);
    nd.s = ns.s >> imm;

    set_gpr(exu, rd, nd.u);

    DBG_LOG(LOG_TRACE, "srai %s, %s, %u\n",
        gpr_name(rd), gpr_name(rs1), imm);
}

DECL_GROUP_HANDLER(sri)
{
    if (i->req.inst.i.imm_11_0 & 0b10000000000) {
        CALL_HANDLER(srai, exu, i);
    } else {
        CALL_HANDLER(srli, exu, i);
    }
}

DECL_GROUP_HANDLER(alui)
{
    static inst_handler_t alui_handlers[8] = {
        [ALUI_FUNCT3_ADDI] = GET_HANDLER(addi),
        [ALUI_FUNCT3_SLTI] = GET_HANDLER(slti),
        [ALUI_FUNCT3_SLTIU] = GET_HANDLER(sltiu),
        [ALUI_FUNCT3_XORI] = GET_HANDLER(xori),
        [ALUI_FUNCT3_ORI] = GET_HANDLER(ori),
        [ALUI_FUNCT3_ANDI] = GET_HANDLER(andi),
        [ALUI_FUNCT3_SLI] = GET_HANDLER(slli),
        [ALUI_FUNCT3_SRI] = GET_HANDLER(sri)
    };

    alui_handlers[i->req.inst.i.funct3](exu, i);
}

DECL_INST_HANDLER(add)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) + get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "add %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_INST_HANDLER(sub)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) - get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "sub %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_GROUP_HANDLER(add_sub)
{
    if (i->req.inst.r.funct7 & 0b100000) {
        CALL_HANDLER(sub, exu, i);
    } else {
        CALL_HANDLER(add, exu, i);
    }
}

DECL_INST_HANDLER(sll)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;

    u32 s1 = get_gpr(exu, rs1);
    u32 s2 = get_gpr(exu, rs2) & 0b11111;
    set_gpr(exu, rd, s1 << s2);

    DBG_LOG(LOG_TRACE, "sll %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_INST_HANDLER(slt)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;

    i32 s1, s2;
    s1.u = get_gpr(exu, rs1);
    s2.u = get_gpr(exu, rs2);

    set_gpr(exu, rd, s1.s < s2.s ? 1 : 0);

    DBG_LOG(LOG_TRACE, "slt %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_INST_HANDLER(sltu)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) < get_gpr(exu, rs2) ? 1 : 0);

    DBG_LOG(LOG_TRACE, "sltu %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_INST_HANDLER(xor)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) ^ get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "xor %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_INST_HANDLER(srl)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;

    u32 s1 = get_gpr(exu, rs1);
    u32 s2 = get_gpr(exu, rs2) & 0b11111;
    set_gpr(exu, rd, s1 >> s2);

    DBG_LOG(LOG_TRACE, "srl %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_INST_HANDLER(sra)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;

    u32 s2 = get_gpr(exu, rs2) & 0b11111;

    i32 s1, d;
    s1.u = get_gpr(exu, rs1);
    d.s = s1.s >> s2;

    set_gpr(exu, rd, d.u);

    DBG_LOG(LOG_TRACE, "sra %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_GROUP_HANDLER(sr)
{
    if (i->req.inst.r.funct7 & 0b100000) {
        CALL_HANDLER(sra, exu, i);
    } else {
        CALL_HANDLER(srl, exu, i);
    }
}

DECL_INST_HANDLER(or)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) | get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "or %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
}

DECL_INST_HANDLER(and)
{
    u32 rd = i->req.inst.r.rd;
    u32 rs1 = i->req.inst.r.rs1;
    u32 rs2 = i->req.inst.r.rs2;
    set_gpr(exu, rd, get_gpr(exu, rs1) & get_gpr(exu, rs2));

    DBG_LOG(LOG_TRACE, "and %s, %s, %s\n",
        gpr_name(rd), gpr_name(rs1), gpr_name(rs2));
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

    alu_handlers[i->req.inst.i.funct3](exu, i);
}

DECL_INST_HANDLER(fence)
{
    DBG_LOG(LOG_TRACE, "fence\n");
}

DECL_INST_HANDLER(ecall)
{
    DBG_LOG(LOG_TRACE, "ecall\n");
}

DECL_INST_HANDLER(ebreak)
{
    DBG_LOG(LOG_TRACE, "ebreak\n");
}

DECL_GROUP_HANDLER(sys)
{
    if (i->req.inst.i.imm_11_0 == 0b000000000000) {
        CALL_HANDLER(ecall, exu, i);
    } else if (i->req.inst.i.imm_11_0 == 0b000000000001) {
        CALL_HANDLER(ebreak, exu, i);
    } else {
        DBG_CHECK(0);
    }
}

static inline void print_split_line(bool newline)
{
    DBG_LOG(LOG_TRACE, newline ?
        "------------------------------------------------------------------------------\n\n" :
        "------------------------------------------------------------------------------\n"
    );
}

static void exu_dump(exu_t *exu, u32 pc)
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

void exu_exec(exu_t *exu, iexec_if_t *i)
{
    static inst_handler_t opcode_handlers[128] = {
        [OPCODE_LUI] = GET_HANDLER(lui),
        [OPCODE_AUIPC] = GET_HANDLER(auipc),
        [OPCODE_JAL] = GET_HANDLER(jal),
        [OPCODE_JALR] = GET_HANDLER(jalr),
        [OPCODE_BRANCH] = GET_HANDLER(branch),
        [OPCODE_LOAD] = GET_HANDLER(load),
        [OPCODE_STORE] = GET_HANDLER(store),
        [OPCODE_ALUI] = GET_HANDLER(alui),
        [OPCODE_ALU] = GET_HANDLER(alu),
        [OPCODE_MEM] = GET_HANDLER(fence),
        [OPCODE_SYSTEM] = GET_HANDLER(sys)
    };

    if (i->req.is_boot_code) {
        dbg_disable_log_module(LOG_TRACE);
    }

    i->rsp.taken = false;
    i->rsp.abs_jump = false;

    u32 opcode = i->req.inst.raw & 0b1111111;
    inst_handler_t handler = opcode_handlers[opcode];
    if (handler) {
        print_split_line(false);
        exu_dump(exu, i->req.pc);
        handler(exu, i);
        print_split_line(true);
    } else {
        DBG_CHECK(0);
    }

    if (i->req.is_boot_code) {
        dbg_enable_log_module(LOG_TRACE);
    }
}

void exu_construct(exu_t *exu, lsu_t *lsu)
{
    exu->lsu = lsu;
}

void exu_reset(exu_t *exu)
{
    exu->gpr[0] = 0;
    for (int i = 1; i < 32; i++) {
        exu->gpr[i] = (u32)rand();
    }
}

void exu_free(exu_t *exu) {}