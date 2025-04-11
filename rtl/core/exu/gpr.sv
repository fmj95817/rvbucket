`include "isa.svh"

interface exu_gpr_r_if_t;
    logic                  vld;
    logic [`RV_GPR_AW-1:0] addr;
    logic [`RV_XLEN-1:0]   data;

    modport mst(output vld, addr, input data);
    modport slv(input vld, addr, output data);
endinterface

interface exu_gpr_w_if_t;
    logic                  wen;
    logic [`RV_GPR_AW-1:0] addr;
    logic [`RV_XLEN-1:0]   data;

    modport mst(output wen, addr, data);
    modport slv(input wen, addr, data);
endinterface

module exu_gpr(
    input logic        clk,
    exu_gpr_r_if_t.slv gpr_r1_slv,
    exu_gpr_r_if_t.slv gpr_r2_slv,
    exu_gpr_w_if_t.slv gpr_w_slv
);
    logic [`RV_XLEN-1:0] gprs[0:2**`RV_GPR_AW-1];
    assign gprs[0] = {`RV_XLEN{1'b0}};

    assign gpr_r1_slv.data = gprs[gpr_r1_slv.addr] & {`RV_XLEN{gpr_r1_slv.vld}};
    assign gpr_r2_slv.data = gprs[gpr_r2_slv.addr] & {`RV_XLEN{gpr_r2_slv.vld}};

    for (genvar i = 1; i < 2**`RV_GPR_AW; i++) begin : GEN_GPR
        always_ff @(posedge clk) begin
            if ((i == gpr_w_slv.addr) & gpr_w_slv.wen)
                gprs[i] <= gpr_w_slv.data;
        end
    end
endmodule