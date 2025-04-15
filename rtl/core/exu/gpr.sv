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

module exu_gpr_rw_mux #(
    parameter CHN_NUM = 2
)(
    input logic        chn_sels[CHN_NUM],
    exu_gpr_r_if_t.slv gpr_r1_slvs[CHN_NUM],
    exu_gpr_r_if_t.slv gpr_r2_slvs[CHN_NUM],
    exu_gpr_w_if_t.slv gpr_w_slvs[CHN_NUM],
    exu_gpr_r_if_t.mst gpr_r1_mst,
    exu_gpr_r_if_t.mst gpr_r2_mst,
    exu_gpr_w_if_t.mst gpr_w_mst
);
    logic                  r1_vld_arr[CHN_NUM];
    logic [`RV_GPR_AW-1:0] r1_addr_arr[CHN_NUM];
    logic                  r2_vld_arr[CHN_NUM];
    logic [`RV_GPR_AW-1:0] r2_addr_arr[CHN_NUM];
    logic                  w_wen_arr[CHN_NUM];
    logic [`RV_GPR_AW-1:0] w_addr_arr[CHN_NUM];
    logic [`RV_XLEN-1:0]   w_data_arr[CHN_NUM];

    for (genvar i = 0; i < CHN_NUM; i++) begin
        assign gpr_r1_slvs[i].data = {`RV_XLEN{chn_sels[i]}} & gpr_r1_mst.data;
        assign gpr_r2_slvs[i].data = {`RV_XLEN{chn_sels[i]}} & gpr_r2_mst.data;

        assign r1_vld_arr[i] = gpr_r1_slvs[i].vld;
        assign r1_addr_arr[i] = gpr_r1_slvs[i].addr;
        assign r2_vld_arr[i] = gpr_r2_slvs[i].vld;
        assign r2_addr_arr[i] = gpr_r2_slvs[i].addr;
        assign w_wen_arr[i] = gpr_w_slvs[i].wen;
        assign w_addr_arr[i] = gpr_w_slvs[i].addr;
        assign w_data_arr[i] = gpr_w_slvs[i].data;
    end

    always_comb begin
        gpr_r1_mst.vld = 1'b0;
        gpr_r1_mst.addr = {`RV_GPR_AW{1'b0}};
        gpr_r2_mst.vld = 1'b0;
        gpr_r2_mst.addr = {`RV_GPR_AW{1'b0}};
        gpr_w_mst.wen = 1'b0;
        gpr_w_mst.addr = {`RV_GPR_AW{1'b0}};
        gpr_w_mst.data = {`RV_XLEN{1'b0}};

        for (int i = 0; i < CHN_NUM; i++) begin
            gpr_r1_mst.vld |= (chn_sels[i] & r1_vld_arr[i]);
            gpr_r1_mst.addr |= ({`RV_GPR_AW{chn_sels[i]}} & r1_addr_arr[i]);
            gpr_r2_mst.vld |= (chn_sels[i] & r2_vld_arr[i]);
            gpr_r2_mst.addr |= ({`RV_GPR_AW{chn_sels[i]}} & r2_addr_arr[i]);
            gpr_w_mst.wen |= (chn_sels[i] & w_wen_arr[i]);
            gpr_w_mst.addr |= ({`RV_GPR_AW{chn_sels[i]}} & w_addr_arr[i]);
            gpr_w_mst.data |= ({`RV_XLEN{chn_sels[i]}} & w_data_arr[i]);
        end
    end
endmodule