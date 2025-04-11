`ifndef ISA_SVH
`define ISA_SVH

`define RV_PC_SIZE   32
`define RV_IR_SIZE   32
`define RV_XLEN      32
`define RV_AW        32
`define RV_OPC_SIZE  7
`define RV_GPR_AW    5

typedef enum logic [6:0] {
    OPCODE_LUI = 7'b0110111,
    OPCODE_AUIPC = 7'b0010111,
    OPCODE_JAL = 7'b1101111,
    OPCODE_JALR = 7'b1100111,
    OPCODE_BRANCH = 7'b1100011,
    OPCODE_LOAD = 7'b0000011,
    OPCODE_STORE = 7'b0100011,
    OPCODE_ALUI = 7'b0010011,
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
    ALUI_FUNCT3_ADDI = 3'b000,
    ALUI_FUNCT3_SLTI = 3'b010,
    ALUI_FUNCT3_SLTIU = 3'b011,
    ALUI_FUNCT3_XORI = 3'b100,
    ALUI_FUNCT3_ORI = 3'b110,
    ALUI_FUNCT3_ANDI = 3'b111,
    ALUI_FUNCT3_SLI = 3'b001,
    ALUI_FUNCT3_SRI = 3'b101
} rv32i_alui_funct3_t;

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

typedef struct packed {
    logic [24:0] args;
    logic [6:0] opcode;
} rv32i_inst_base_t;

typedef struct packed {
    logic [6:0] funct7;
    logic [4:0] rs2;
    logic [4:0] rs1;
    logic [2:0] funct3;
    logic [4:0] rd;
    logic [6:0] opcode;
} rv32i_inst_r_t;

typedef struct packed {
    logic [11:0] imm_11_0;
    logic [4:0] rs1;
    logic [2:0] funct3;
    logic [4:0] rd;
    logic [6:0] opcode;
} rv32i_inst_i_t;

typedef struct packed {
    logic [6:0] imm_11_5;
    logic [4:0] rs2;
    logic [4:0] rs1;
    logic [2:0] funct3;
    logic [4:0] imm_4_0;
    logic [6:0] opcode;
} rv32i_inst_s_t;

typedef struct packed {
    logic       imm_12;
    logic [5:0] imm_10_5;
    logic [4:0] rs2;
    logic [4:0] rs1;
    logic [2:0] funct3;
    logic [3:0] imm_4_1;
    logic       imm_11;
    logic [6:0] opcode;
} rv32i_inst_b_t;

typedef struct packed {
    logic [19:0] imm_31_12;
    logic [4:0] rd;
    logic [6:0] opcode;
} rv32i_inst_u_t;

typedef struct packed {
    logic       imm_20;
    logic [9:0] imm_10_1;
    logic       imm_11;
    logic [7:0] imm_19_12;
    logic [4:0] rd;
    logic [6:0] opcode;
} rv32i_inst_j_t;

typedef union packed {
    rv32i_inst_r_t r;
    rv32i_inst_i_t i;
    rv32i_inst_s_t s;
    rv32i_inst_b_t b;
    rv32i_inst_u_t u;
    rv32i_inst_j_t j;
    rv32i_inst_base_t base;
    logic [`RV_IR_SIZE-1:0] raw;
} rv32i_inst_t;

`endif