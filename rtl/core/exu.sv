
`include "isa.svh"

typedef enum logic {
    ALU_OPCODE_ADD = 1'b0,
    ALU_OPCODE_SUB = 1'b1
} alu_opcode_t;

interface alu_op_if_t;
    logic                opcode;
    logic [`RV_XLEN-1:0] src1;
    logic [`RV_XLEN-1:0] src2;
    logic [`RV_XLEN-1:0] dst;

    modport master (output opcode, src1, src2, input dst);
    modport slave (input opcode, src1, src2, output dst);
endinterface

interface gprf_rw_if_t;
    logic [`RV_GPR_AW-1:0] raddr1;
    logic [`RV_XLEN-1:0]   rdata1;
    logic [`RV_GPR_AW-1:0] raddr2;
    logic [`RV_XLEN-1:0]   rdata2;
    logic [`RV_GPR_AW-1:0] waddr;
    logic [`RV_XLEN-1:0]   wdata;
    logic                  wen;

    modport master (
        output raddr1, raddr2, waddr, wdata, wen,
        input rdata1, rdata2
    );

    modport slave (
        input raddr1, raddr2, waddr, wdata, wen,
        output rdata1, rdata2
    );
endinterface

module alu(alu_op_if_t.slave alu_op);
    tri alu_op_add = alu_op.opcode == ALU_OPCODE_ADD;
    tri alu_op_sub = alu_op.opcode == ALU_OPCODE_SUB;

    assign alu_op.dst = (
        ({`RV_XLEN{alu_op_add}} & (alu_op.src1 + alu_op.src2)) |
        ({`RV_XLEN{alu_op_sub}} & (alu_op.src1 - alu_op.src2))
    );
endmodule

module gprf(
    input               clk,
    input               rst_n,
    gprf_rw_if_t.slave  gprf_rw
);
    logic [`RV_XLEN-1:0] gprs[0:2**`RV_GPR_AW-1];
    assign gprs[0] = {`RV_XLEN{1'b0}};

    for (genvar i = 1; i < 2**`RV_GPR_AW; i++) begin
        always_ff @(posedge clk) begin
            if ((i == gprf_rw.waddr) & gprf_rw.wen)
                gprs[i] <= #1 gprf_rw.wdata;
        end
    end

    assign gprf_rw.rdata1 = gprs[gprf_rw.raddr1];
    assign gprf_rw.rdata2 = gprs[gprf_rw.raddr2];
endmodule

module lui_handler(
    rv32i_u_dec_if_t.slave  u_dec,
    alu_op_if_t.master      alu_op,
    gprf_rw_if_t.master     gprf_rw
);
    assign gprf_rw.waddr = u_dec.rd;
    assign gprf_rw.wdata = u_dec.imm;
    assign gprf_rw.wen = 1'b1;
endmodule

module inst_handler_mux(
    input [`RV_OPCODE_SIZE-1:0] opcode,
    input                       iexec_req_hsk,
    alu_op_if_t.slave           lui_alu_op,
    gprf_rw_if_t.slave          lui_gprf_rw,
    alu_op_if_t.master          alu_op,
    gprf_rw_if_t.master         gprf_rw
);
    logic gpr_wen;
    always_comb begin
    case (opcode)
        OPCODE_LUI: begin
            alu_op.opcode = lui_alu_op.opcode;
            alu_op.src1 = lui_alu_op.src1;
            alu_op.src2 = lui_alu_op.src2;

            gprf_rw.raddr1 = lui_gprf_rw.raddr1;
            gprf_rw.raddr2 = lui_gprf_rw.raddr2;
            gprf_rw.waddr = lui_gprf_rw.waddr;
            gprf_rw.wdata = lui_gprf_rw.wdata;
            gpr_wen = lui_gprf_rw.wen;
        end

        default: begin
            alu_op.opcode = 1'bx;
            alu_op.src1 = {`RV_XLEN{1'bx}};
            alu_op.src2 = {`RV_XLEN{1'bx}};

            gprf_rw.raddr1 = {`RV_GPR_AW{1'bx}};
            gprf_rw.raddr2 = {`RV_GPR_AW{1'bx}};
            gprf_rw.waddr = {`RV_GPR_AW{1'bx}};
            gprf_rw.wdata = {`RV_XLEN{1'bx}};
            gpr_wen = 1'b0;
        end
    endcase
    end

    assign gprf_rw.wen = gpr_wen & iexec_req_hsk;

    assign lui_gprf_rw.rdata1 = lui_gprf_rw.rdata1;
    assign lui_gprf_rw.rdata2 = lui_gprf_rw.rdata2;

    assign lui_alu_op.dst = alu_op.dst;
endmodule

module exu(
    input             clk,
    input             rst_n,
    iexec_if_t.slave  iexec,
    ldst_if_t.master  ldst_src
);
    tri iexec_req_hsk = iexec.req_vld & iexec.req_rdy;

    /* ISA decoder */
    tri [`RV_IR_SIZE-1:0] ir = iexec.req_pkt.ir;
    tri [`RV_OPCODE_SIZE-1:0] opcode;

    rv32i_r_dec_if_t r_dec();
    rv32i_i_dec_if_t i_dec();
    rv32i_s_dec_if_t s_dec();
    rv32i_b_dec_if_t b_dec();
    rv32i_u_dec_if_t u_dec();
    rv32i_j_dec_if_t j_dec();

    rv32i_isa_dec u_rv32i_isa_dec(.*);

    /* ALU & GPR */
    alu_op_if_t alu_op();
    gprf_rw_if_t gprf_rw();

    alu u_alu(.*);
    gprf u_gprf(.*);

    /* lui */
    alu_op_if_t lui_alu_op();
    gprf_rw_if_t lui_gprf_rw();
    lui_handler u_lui_handler(u_dec, lui_alu_op, lui_gprf_rw);

    /* inst op mux */
    inst_handler_mux u_inst_handler_mux(.*);

    assign iexec.req_rdy = 1'b1;
    assign ldst_src.req_vld = 1'b0;
endmodule