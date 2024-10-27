`ifndef DP_SVH
`define DP_SVH

`define ALU_OPC_SIZE 4

typedef enum logic [`ALU_OPC_SIZE-1:0] {
    ALU_OPCODE_ADD = 4'd0,
    ALU_OPCODE_SUB = 4'd1,
    ALU_OPCODE_LESS_S = 4'd2,
    ALU_OPCODE_LESS_U = 4'd3,
    ALU_OPCODE_XOR = 4'd4,
    ALU_OPCODE_OR = 4'd5,
    ALU_OPCODE_AND = 4'd6,
    ALU_OPCODE_SL = 4'd7,
    ALU_OPCODE_SRL = 4'd8,
    ALU_OPCODE_SRA = 4'd9
} alu_opcode_t;

`endif