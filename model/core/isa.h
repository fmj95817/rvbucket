#ifndef ISA_H
#define ISA_H

#include "base/types.h"

#pragma pack(1)
typedef enum rv32i_opcode {
    OPCODE_LUI = 0b0110111,
    OPCODE_AUIPC = 0b0010111,
    OPCODE_JAL = 0b1101111,
    OPCODE_JALR = 0b1100111,
    OPCODE_BRANCH = 0b1100011,
    OPCODE_LOAD = 0b0000011,
    OPCODE_STORE = 0b0100011,
    OPCODE_ALUI = 0b0010011,
    OPCODE_ALU = 0b0110011,
    OPCODE_MEM = 0b0001111,
    OPCODE_SYSTEM = 0b1110011
} rv32i_opcode_t;

typedef enum rv32i_branch_funct3 {
    BRANCH_FUNCT3_BEQ = 0b000,
    BRANCH_FUNCT3_BNE = 0b001,
    BRANCH_FUNCT3_BLT = 0b100,
    BRANCH_FUNCT3_BGE = 0b101,
    BRANCH_FUNCT3_BLTU = 0b110,
    BRANCH_FUNCT3_BGEU = 0b111
} rv32i_branch_funct3_t;

typedef enum rv32i_alui_funct3 {
    ALUI_FUNCT3_ADDI = 0b000,
    ALUI_FUNCT3_SLTI = 0b010,
    ALUI_FUNCT3_SLTIU = 0b011,
    ALUI_FUNCT3_XORI = 0b100,
    ALUI_FUNCT3_ORI = 0b110,
    ALUI_FUNCT3_ANDI = 0b111,
    ALUI_FUNCT3_SLI = 0b001,
    ALUI_FUNCT3_SRI = 0b101
} rv32i_alui_funct3_t;

typedef enum rv32i_alu_funct3 {
    ALU_FUNCT3_ADD_SUB = 0b000,
    ALU_FUNCT3_SL = 0b001,
    ALU_FUNCT3_SLT = 0b010,
    ALU_FUNCT3_SLTU = 0b011,
    ALU_FUNCT3_XOR = 0b100,
    ALU_FUNCT3_SR = 0b101,
    ALU_FUNCT3_OR = 0b110,
    ALU_FUNCT3_AND = 0b111
} rv32i_alu_funct3_t;

typedef enum rv32i_load_funct3 {
    LOAD_FUNCT3_LB = 0b000,
    LOAD_FUNCT3_LH = 0b001,
    LOAD_FUNCT3_LW = 0b010,
    LOAD_FUNCT3_LBU = 0b100,
    LOAD_FUNCT3_LHU = 0b101
} rv32i_load_funct3_t;

typedef enum rv32i_store_funct3 {
    STORE_FUNCT3_SB = 0b000,
    STORE_FUNCT3_SH = 0b001,
    STORE_FUNCT3_SW = 0b010
} rv32i_store_funct3_t;

typedef struct rv32i_inst_r {
    u32 opcode : 7;
    u32 rd : 5;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 rs2 : 5;
    u32 funct7 : 7;
} rv32i_inst_r_t;

typedef struct rv32i_inst_i {
    u32 opcode : 7;
    u32 rd : 5;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 imm_11_0 : 12;
} rv32i_inst_i_t;

typedef struct rv32i_inst_s {
    u32 opcode : 7;
    u32 imm_4_0 : 5;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 rs2 : 5;
    u32 imm_11_5 : 7;
} rv32i_inst_s_t;

typedef struct rv32i_inst_b {
    u32 opcode : 7;
    u32 imm_11 : 1;
    u32 imm_4_1 : 4;
    u32 funct3 : 3;
    u32 rs1 : 5;
    u32 rs2 : 5;
    u32 imm_10_5 : 6;
    u32 imm_12 : 1;
} rv32i_inst_b_t;

typedef struct rv32i_inst_u {
    u32 opcode : 7;
    u32 rd : 5;
    u32 imm_31_12 : 20;
} rv32i_inst_u_t;

typedef struct rv32i_inst_j {
    u32 opcode : 7;
    u32 rd : 5;
    u32 imm_19_12 : 8;
    u32 imm_11 : 1;
    u32 imm_10_1 : 10;
    u32 imm_20 : 1;
} rv32i_inst_j_t;

typedef struct rv32i_inst_bas {
    u32 opcode : 7;
    u32 args: 25;
} rv32i_inst_base_t;

typedef union rv32i_inst {
    rv32i_inst_r_t r;
    rv32i_inst_i_t i;
    rv32i_inst_s_t s;
    rv32i_inst_b_t b;
    rv32i_inst_u_t u;
    rv32i_inst_j_t j;
    rv32i_inst_base_t base;
    u32 raw;
} rv32i_inst_t;
#pragma pack()

#endif