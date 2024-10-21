typedef struct packed {
    logic [6:0] opcode;
    logic [4:0] rd;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [4:0] rs2;
    logic [6:0] funct7;
} rv32i_inst_r_t;

typedef struct packed {
    logic [6:0] opcode;
    logic [4:0] rd;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [11:0] imm_11_0;
} rv32i_inst_i_t;

typedef struct packed {
    logic [6:0] opcode;
    logic [4:0] imm_4_0;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [4:0] rs2;
    logic [6:0] imm_11_5;
} rv32i_inst_s_t;

typedef struct packed {
    logic [6:0] opcode;
    logic [0:0] imm_11;
    logic [3:0] imm_4_1;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [4:0] rs2;
    logic [5:0] imm_10_5;
    logic [0:0] imm_12;
} rv32i_inst_b_t;

typedef struct packed {
    logic [6:0] opcode;
    logic [4:0] rd;
    logic [19:0] imm_31_12;
} rv32i_inst_u_t;

typedef struct packed {
    logic [6:0] opcode;
    logic [4:0] rd;
    logic [7:0] imm_19_12;
    logic [0:0] imm_11;
    logic [9:0] imm_10_1;
    logic [0:0] imm_20;
} rv32i_inst_j_t;

typedef union packed {
    rv32i_inst_r_t r;
    rv32i_inst_i_t i;
    rv32i_inst_s_t s;
    rv32i_inst_b_t b;
    rv32i_inst_u_t u;
    rv32i_inst_j_t j;
    logic [31:0] raw;
} rv32i_inst_t;