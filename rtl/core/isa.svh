`ifndef ISA_SVH
`define ISA_SVH

`define RV_IR_SIZE 32
`define RV_OPCODE_SIZE 7
`define RV_XLEN    32
`define RV_GPR_AW  5

typedef enum logic [6:0] {
    OPCODE_LUI = 7'b0110111,
    OPCODE_AUIPC = 7'b0010111,
    OPCODE_JAL = 7'b1101111,
    OPCODE_JALR = 7'b1100111,
    OPCODE_BRANCH = 7'b1100011,
    OPCODE_LOAD = 7'b0000011,
    OPCODE_STORE = 7'b0100011,
    OPCODE_ALU_IMM = 7'b0010011,
    OPCODE_ALU = 7'b0110011,
    OPCODE_MEM = 7'b0001111,
    OPCODE_SYSTEM = 7'b1110011
} rv32i_opcode_t;

typedef enum logic [2:0] {
    BRANCH_FUNCT3_BEQ = 3'b000,
    BRANCH_FUNCT3_BNE = 3'b001,
    BRANCH_FUNCT3_BLT = 3'b100,
    BRANCH_FUNCT3_BGE = 3'b101,
    BRANCH_FUNCT3_BLTU = 3'b110,
    BRANCH_FUNCT3_BGEU = 3'b111
} rv32i_branch_funct3_t;

typedef enum logic [2:0] {
    ALU_IMM_FUNCT3_ADDI = 3'b000,
    ALU_IMM_FUNCT3_SLTI = 3'b010,
    ALU_IMM_FUNCT3_SLTIU = 3'b011,
    ALU_IMM_FUNCT3_XORI = 3'b100,
    ALU_IMM_FUNCT3_ORI = 3'b110,
    ALU_IMM_FUNCT3_ANDI = 3'b111,
    ALU_IMM_FUNCT3_SLI = 3'b001,
    ALU_IMM_FUNCT3_SRI = 3'b101
} rv32i_alu_imm_funct3_t;

typedef enum logic [2:0] {
    ALU_FUNCT3_ADD_SUB = 3'b000,
    ALU_FUNCT3_SL = 3'b001,
    ALU_FUNCT3_SLT = 3'b010,
    ALU_FUNCT3_SLTU = 3'b011,
    ALU_FUNCT3_XOR = 3'b100,
    ALU_FUNCT3_SR = 3'b101,
    ALU_FUNCT3_OR = 3'b110,
    ALU_FUNCT3_AND = 3'b111
} rv32i_alu_funct3_t;

typedef enum logic [2:0] {
    LOAD_FUNCT3_LB = 3'b000,
    LOAD_FUNCT3_LH = 3'b001,
    LOAD_FUNCT3_LW = 3'b010,
    LOAD_FUNCT3_LBU = 3'b100,
    LOAD_FUNCT3_LHU = 3'b101
} rv32i_load_funct3_t;

typedef enum logic [2:0] {
    STORE_FUNCT3_SB = 3'b000,
    STORE_FUNCT3_SH = 3'b001,
    STORE_FUNCT3_SW = 3'b010
} rv32i_store_funct3_t;

`endif