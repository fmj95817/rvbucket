#ifndef ISA_H
#define ISA_H

#include "base/types.h"

#define RV32G_GPR_NUM 32

#pragma pack(1)
typedef enum rv32g_priv {
    RV32G_PRIV_USER = 0b00,
    RV32G_PRIV_SUPERVISOR = 0b01,
    RV32G_PRIV_MACHINE = 0b11,
    RV32G_PRIV_MAX = RV32G_PRIV_MACHINE
} rv32g_priv_t;

typedef enum rv32g_opcode {
    OPCODE_LUI = 0b0110111,
    OPCODE_AUIPC = 0b0010111,
    OPCODE_JAL = 0b1101111,
    OPCODE_JALR = 0b1100111,
    OPCODE_BRANCH = 0b1100011,
    OPCODE_LOAD = 0b0000011,
    OPCODE_STORE = 0b0100011,
    OPCODE_ALUI = 0b0010011,
    OPCODE_ALU = 0b0110011,
    OPCODE_MISC_MEM = 0b0001111,
    OPCODE_SYSTEM = 0b1110011,
    OPCODE_AMO = 0b0101111
} rv32g_opcode_t;

typedef enum rv32g_branch_funct3 {
    BRANCH_FUNCT3_BEQ = 0b000,
    BRANCH_FUNCT3_BNE = 0b001,
    BRANCH_FUNCT3_BLT = 0b100,
    BRANCH_FUNCT3_BGE = 0b101,
    BRANCH_FUNCT3_BLTU = 0b110,
    BRANCH_FUNCT3_BGEU = 0b111
} rv32g_branch_funct3_t;

typedef enum rv32g_alui_funct3 {
    ALUI_FUNCT3_ADDI = 0b000,
    ALUI_FUNCT3_SLTI = 0b010,
    ALUI_FUNCT3_SLTIU = 0b011,
    ALUI_FUNCT3_XORI = 0b100,
    ALUI_FUNCT3_ORI = 0b110,
    ALUI_FUNCT3_ANDI = 0b111,
    ALUI_FUNCT3_SLI = 0b001,
    ALUI_FUNCT3_SRI = 0b101
} rv32g_alui_funct3_t;

typedef enum rv32g_alu_funct37 {
    ALU_FUNCT37_ADD = 0b0000000000,
    ALU_FUNCT37_SUB = 0b0000100000,
    ALU_FUNCT37_SL = 0b0010000000,
    ALU_FUNCT37_SLT = 0b0100000000,
    ALU_FUNCT37_SLTU = 0b0110000000,
    ALU_FUNCT37_XOR = 0b1000000000,
    ALU_FUNCT37_SRL = 0b1010000000,
    ALU_FUNCT37_SRA = 0b1010100000,
    ALU_FUNCT37_OR = 0b1100000000,
    ALU_FUNCT37_AND = 0b1110000000,
    ALU_FUNCT37_MUL = 0b0000000001,
    ALU_FUNCT37_MULH = 0b0010000001,
    ALU_FUNCT37_MULHSU = 0b0100000001,
    ALU_FUNCT37_MULHU = 0b0110000001,
    ALU_FUNCT37_DIV = 0b1000000001,
    ALU_FUNCT37_DIVU = 0b1010000001,
    ALU_FUNCT37_REM = 0b1100000001,
    ALU_FUNCT37_REMU = 0b1110000001
} rv32g_alu_funct37_t;

typedef enum rv32g_load_funct3 {
    LOAD_FUNCT3_LB = 0b000,
    LOAD_FUNCT3_LH = 0b001,
    LOAD_FUNCT3_LW = 0b010,
    LOAD_FUNCT3_LBU = 0b100,
    LOAD_FUNCT3_LHU = 0b101
} rv32g_load_funct3_t;

typedef enum rv32g_store_funct3 {
    STORE_FUNCT3_SB = 0b000,
    STORE_FUNCT3_SH = 0b001,
    STORE_FUNCT3_SW = 0b010
} rv32g_store_funct3_t;

typedef enum rv32g_system_funct3 {
    SYSTEM_FUNCT3_PRIV = 0b000,
    SYSTEM_FUNCT3_CSRRW = 0b001,
    SYSTEM_FUNCT3_CSRRS = 0b010,
    SYSTEM_FUNCT3_CSRRC = 0b011,
    SYSTEM_FUNCT3_CSRRWI = 0b101,
    SYSTEM_FUNCT3_CSRRSI = 0b110,
    SYSTEM_FUNCT3_CSRRCI = 0b111
} rv32g_system_funct3_t;

typedef enum rv32g_misc_mem_funct3 {
    MISC_MEM_FUNCT3_FENCE = 0b000,
    MISC_MEM_FUNCT3_FENCE_I = 0b001,
    MISC_MEM_FUNCT3_CBO = 0b010
} rv32g_misc_mem_funct3_t;

typedef enum rv32g_amo_funct375 {
    RV32G_AMO_FUNCT375_LR = 0b01000010,
    RV32G_AMO_FUNCT375_SC = 0b01000011,
    RV32G_AMO_FUNCT375_AMOSWAP = 0b01000001,
    RV32G_AMO_FUNCT375_AMOADD = 0b01000000,
    RV32G_AMO_FUNCT375_AMOXOR = 0b01000100,
    RV32G_AMO_FUNCT375_AMOAND = 0b01001100,
    RV32G_AMO_FUNCT375_AMOOR = 0b01001000,
    RV32G_AMO_FUNCT375_AMOMIN = 0b01010000,
    RV32G_AMO_FUNCT375_AMOMAX = 0b01010100,
    RV32G_AMO_FUNCT375_AMOMINU = 0b01011000,
    RV32G_AMO_FUNCT375_AMOMAXU = 0b01011100
} rv32g_amo_funct375_t;

typedef struct rv32g_inst_r {
    u32 opcode : 7;
    u32 rd : 5;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 rs2 : 5;
    u32 funct7 : 7;
} rv32g_inst_r_t;

typedef struct rv32g_inst_i {
    u32 opcode : 7;
    u32 rd : 5;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 imm_11_0 : 12;
} rv32g_inst_i_t;

typedef struct rv32g_inst_s {
    u32 opcode : 7;
    u32 imm_4_0 : 5;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 rs2 : 5;
    u32 imm_11_5 : 7;
} rv32g_inst_s_t;

typedef struct rv32g_inst_b {
    u32 opcode : 7;
    u32 imm_11 : 1;
    u32 imm_4_1 : 4;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 rs2 : 5;
    u32 imm_10_5 : 6;
    u32 imm_12 : 1;
} rv32g_inst_b_t;

typedef struct rv32g_inst_u {
    u32 opcode : 7;
    u32 rd : 5;
    u32 imm_31_12 : 20;
} rv32g_inst_u_t;

typedef struct rv32g_inst_j {
    u32 opcode : 7;
    u32 rd : 5;
    u32 imm_19_12 : 8;
    u32 imm_11 : 1;
    u32 imm_10_1 : 10;
    u32 imm_20 : 1;
} rv32g_inst_j_t;

typedef struct rv32g_inst_base {
    u32 opcode : 7;
    u32 args: 25;
} rv32g_inst_base_t;

typedef union rv32g_inst {
    rv32g_inst_r_t r;
    rv32g_inst_i_t i;
    rv32g_inst_s_t s;
    rv32g_inst_b_t b;
    rv32g_inst_u_t u;
    rv32g_inst_j_t j;
    rv32g_inst_base_t base;
    u32 raw;
} rv32g_inst_t;
#pragma pack()

static inline u32 rv32g_sign_ext(u32 data, u32 width)
{
    if (data & (1u << (width - 1))) {
        data |= ((~0u) << width);
    }
    return data;
}

static inline u32 u_imm_decode(const rv32g_inst_t *i)
{
    return i->u.imm_31_12 << 12;
}

static inline i32 j_imm_decode(const rv32g_inst_t *i)
{
    u32 src = (i->j.imm_10_1 << 1) |
              (i->j.imm_11 << 11) |
              (i->j.imm_19_12 << 12) |
              (i->j.imm_20 << 20);

    i32 dst = { .u = rv32g_sign_ext(src, 21) };
    return dst;
}

static inline i32 i_imm_decode(const rv32g_inst_t *i)
{
    i32 dst = { .u = rv32g_sign_ext(i->i.imm_11_0, 12) };
    return dst;
}

static inline i32 b_imm_decode(const rv32g_inst_t *i)
{
    u32 src = (i->b.imm_4_1 << 1) |
              (i->b.imm_10_5 << 5) |
              (i->b.imm_11 << 11) |
              (i->b.imm_12 << 12);

    i32 dst = { .u = rv32g_sign_ext(src, 13) };
    return dst;
}

static inline i32 s_imm_decode(const rv32g_inst_t *i)
{
    u32 src = (i->s.imm_4_0) |
              (i->s.imm_11_5 << 5);

    i32 dst = { .u = rv32g_sign_ext(src, 12) };
    return dst;
}

#endif
