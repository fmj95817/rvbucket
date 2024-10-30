`include "isa.svh"

interface rv32i_opc_dec_if;
    logic lui;
    logic auipc;
    logic jal;
    logic jalr;
    logic branch;
    logic load;
    logic store;
    logic alui;
    logic alu;
    logic mem;
    logic system;

    modport master (output lui, auipc, jal, jalr,
        branch, load, store, alui, alu, mem, system);
    modport slave (input lui, auipc, jal, jalr,
        branch, load, store, alui, alu, mem, system);
endinterface

interface rv32i_r_dec_if;
    logic [4:0] rd;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [4:0] rs2;
    logic [6:0] funct7;

    modport master (output rd, funct3, rs1, rs2, funct7);
    modport slave (input rd, funct3, rs1, rs2, funct7);
endinterface

interface rv32i_i_dec_if;
    logic [4:0] rd;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [11:0] imm_11_0;
    logic [`RV_XLEN-1:0] imm;

    modport master (output rd, funct3, rs1, imm_11_0, imm);
    modport slave (input rd, funct3, rs1, imm_11_0, imm);
endinterface

interface rv32i_s_dec_if;
    logic [4:0] imm_4_0;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [4:0] rs2;
    logic [6:0] imm_11_5;
    logic [`RV_XLEN-1:0] imm;

    modport master (output imm_4_0, funct3, rs1, rs2, imm_11_5, imm);
    modport slave (input imm_4_0, funct3, rs1, rs2, imm_11_5, imm);
endinterface

interface rv32i_b_dec_if;
    logic       imm_11;
    logic [3:0] imm_4_1;
    logic [2:0] funct3;
    logic [4:0] rs1;
    logic [4:0] rs2;
    logic [5:0] imm_10_5;
    logic       imm_12;
    logic [`RV_XLEN-1:0] imm;

    modport master (output imm_11, imm_4_1, funct3, rs1, rs2, imm_10_5, imm_12, imm);
    modport slave (input imm_11, imm_4_1, funct3, rs1, rs2, imm_10_5, imm_12, imm);
endinterface

interface rv32i_u_dec_if;
    logic [4:0] rd;
    logic [19:0] imm_31_12;
    logic [`RV_XLEN-1:0] imm;

    modport master (output rd, imm_31_12, imm);
    modport slave (input rd, imm_31_12, imm);
endinterface

interface rv32i_j_dec_if;
    logic [4:0] rd;
    logic [7:0] imm_19_12;
    logic       imm_11;
    logic [9:0] imm_10_1;
    logic       imm_20;
    logic [`RV_XLEN-1:0] imm;

    modport master (output rd, imm_19_12, imm_11, imm_10_1, imm_20, imm);
    modport slave (input rd, imm_19_12, imm_11, imm_10_1, imm_20, imm);
endinterface

module rv32i_isa_dec(
    input  [`RV_IR_SIZE-1:0]   ir,
    rv32i_opc_dec_if.master    opc_dec,
    rv32i_r_dec_if.master      r_dec,
    rv32i_i_dec_if.master      i_dec,
    rv32i_s_dec_if.master      s_dec,
    rv32i_b_dec_if.master      b_dec,
    rv32i_u_dec_if.master      u_dec,
    rv32i_j_dec_if.master      j_dec
);
    /* opcode type */
    tri [`RV_OPC_SIZE-1:0] opcode = ir[`RV_OPC_SIZE-1:0];
    assign opc_dec.lui = (opcode == OPCODE_LUI);
    assign opc_dec.auipc = (opcode == OPCODE_AUIPC);
    assign opc_dec.jal = (opcode == OPCODE_JAL);
    assign opc_dec.jalr = (opcode == OPCODE_JALR);
    assign opc_dec.branch = (opcode == OPCODE_BRANCH);
    assign opc_dec.load = (opcode == OPCODE_LOAD);
    assign opc_dec.store = (opcode == OPCODE_STORE);
    assign opc_dec.alui = (opcode == OPCODE_ALUI);
    assign opc_dec.alu = (opcode == OPCODE_ALU);
    assign opc_dec.mem = (opcode == OPCODE_MEM);
    assign opc_dec.system = (opcode == OPCODE_SYSTEM);

    /* R-Type */
    assign r_dec.rd = ir[11:7];
    assign r_dec.funct3 = ir[14:12];
    assign r_dec.rs1 = ir[19:15];
    assign r_dec.rs2 = ir[24:20];
    assign r_dec.funct7 = ir[31:25];

    /* I-Type */
    assign i_dec.rd = ir[11:7];
    assign i_dec.funct3 = ir[14:12];
    assign i_dec.rs1 = ir[19:15];
    assign i_dec.imm_11_0 = ir[31:20];
    assign i_dec.imm = {
        {(`RV_XLEN-12){i_dec.imm_11_0[11]}},
        i_dec.imm_11_0
    };

    /* S-Type */
    assign s_dec.imm_4_0 = ir[11:7];
    assign s_dec.funct3 = ir[14:12];
    assign s_dec.rs1 = ir[19:15];
    assign s_dec.rs2 = ir[24:20];
    assign s_dec.imm_11_5 = ir[31:25];
    assign s_dec.imm = {
        {(`RV_XLEN-12){s_dec.imm_11_5[6]}},
        s_dec.imm_11_5, s_dec.imm_4_0
    };

    /* B-Type */
    assign b_dec.imm_11 = ir[7];
    assign b_dec.imm_4_1 = ir[11:8];
    assign b_dec.funct3 = ir[14:12];
    assign b_dec.rs1 = ir[19:15];
    assign b_dec.rs2 = ir[24:20];
    assign b_dec.imm_10_5 = ir[30:25];
    assign b_dec.imm_12 = ir[31];
    assign b_dec.imm = {
        {(`RV_XLEN-13){b_dec.imm_12}},
        b_dec.imm_12, b_dec.imm_11,
        b_dec.imm_10_5, b_dec.imm_4_1, 1'b0
    };

    /* U-Type */
    assign u_dec.rd = ir[11:7];
    assign u_dec.imm_31_12 = ir[31:12];
    assign u_dec.imm = {
        u_dec.imm_31_12,
        {12{1'b0}}
    };

    /* J-Type */
    assign j_dec.rd = ir[11:7];
    assign j_dec.imm_19_12 = ir[19:12];
    assign j_dec.imm_11 = ir[20];
    assign j_dec.imm_10_1 = ir[30:21];
    assign j_dec.imm_20 = ir[31];
    assign j_dec.imm = {
        {(`RV_XLEN-21){j_dec.imm_20}},
        j_dec.imm_20, j_dec.imm_19_12,
        j_dec.imm_11, j_dec.imm_10_1, 1'b0
    };
endmodule