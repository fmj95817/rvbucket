#ifndef ISA_H
#define ISA_H

#include "types.h"

typedef enum rv32i_opcode {
    OPCODE_LUI = 0b0110111,
    OPCODE_AUIPC = 0b0010111,
    OPCODE_JAL = 0b1101111,
    OPCODE_JALR = 0b1100111,
    OPCODE_BRANCH = 0b1100011,
    OPCODE_LOAD = 0b0000011,
    OPCODE_STORE = 0b0100011,
    OPCODE_ALU_IMM = 0b0010011,
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

typedef enum rv32i_alu_imm_funct3 {
    ALU_IMM_FUNCT3_ADDI = 0b000,
    ALU_IMM_FUNCT3_SLTI = 0b010,
    ALU_IMM_FUNCT3_SLTIU = 0b011,
    ALU_IMM_FUNCT3_XORI = 0b100,
    ALU_IMM_FUNCT3_ORI = 0b110,
    ALU_IMM_FUNCT3_ANDI = 0b111,
    ALU_IMM_FUNCT3_SLI = 0b001,
    ALU_IMM_FUNCT3_SRI = 0b101
} rv32i_alu_imm_funct3_t;

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

typedef union rv32i_inst {
    struct {
        u32 opcode : 7;

        union {
            struct {
                u32 rd : 5;
                u32 funct3 : 3;
                u32 rs1 : 5;
                u32 rs2 : 5;
                u32 funct7 : 7;
            } r_type;

            struct {
                u32 rd : 5;
                u32 funct3 : 3;
                u32 rs1 : 5;
                u32 imm_11_0 : 12;
            } i_type;

            struct {
                u32 imm_4_0 : 5;
                u32 funct3 : 3;
                u32 rs1 : 5;
                u32 rs2 : 5;
                u32 imm_11_5 : 7;
            } s_type;

            struct {
                u32 imm_11 : 1;
                u32 imm_4_1 : 4;
                u32 funct3 : 3;
                u32 rs1 : 5;
                u32 rs2 : 5;
                u32 imm_10_5 : 6;
                u32 imm_12 : 1;
            } b_type;

            struct {
                u32 rd : 5;
                u32 imm_31_12 : 20;
            } u_type;

            struct {
                u32 rd : 5;
                u32 imm_19_12 : 8;
                u32 imm_11 : 1;
                u32 imm_10_1 : 10;
                u32 imm_20 : 1;
            } j_type;
        } args;

    } encoding;

    u32 raw;
} rv32i_inst_t;

#endif