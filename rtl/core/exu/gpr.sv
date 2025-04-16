`include "isa.svh"

interface exu_gpr_if_t;
    logic [`RV_GPR_AW-1:0] ra1;
    logic [`RV_XLEN-1:0]   rd1;
    logic [`RV_GPR_AW-1:0] ra2;
    logic [`RV_XLEN-1:0]   rd2;
    logic                  wen;
    logic [`RV_GPR_AW-1:0] wa;
    logic [`RV_XLEN-1:0]   wd;

    modport mst(output ra1, ra2, wen, wa, wd, input rd1, rd2);
    modport slv(input ra1, ra2, wen, wa, wd, output rd1, rd2);
endinterface

module exu_gpr(
    input logic      clk,
    exu_gpr_if_t.slv gpr_slv
);
    logic [`RV_XLEN-1:0] gprs[0:2**`RV_GPR_AW-1];
    assign gprs[0] = {`RV_XLEN{1'b0}};

    assign gpr_slv.rd1 = gprs[gpr_slv.ra1];
    assign gpr_slv.rd2 = gprs[gpr_slv.ra2];

    for (genvar i = 1; i < 2**`RV_GPR_AW; i++) begin : GEN_GPR
        always_ff @(posedge clk) begin
            if ((i == gpr_slv.wa) & gpr_slv.wen)
                gprs[i] <= gpr_slv.wd;
        end
    end
endmodule

module exu_gpr_rw_mux #(
    parameter CHN_NUM = 2
)(
    input logic      chn_sels[CHN_NUM],
    exu_gpr_if_t.slv gpr_src_slvs[CHN_NUM],
    exu_gpr_if_t.mst gpr_dst_mst
);
    logic [`RV_GPR_AW-1:0] r1_addr_arr[CHN_NUM];
    logic [`RV_GPR_AW-1:0] r2_addr_arr[CHN_NUM];
    logic                  w_wen_arr[CHN_NUM];
    logic [`RV_GPR_AW-1:0] w_addr_arr[CHN_NUM];
    logic [`RV_XLEN-1:0]   w_data_arr[CHN_NUM];

    for (genvar i = 0; i < CHN_NUM; i++) begin
        assign gpr_src_slvs[i].rd1 = {`RV_XLEN{chn_sels[i]}} & gpr_dst_mst.rd1;
        assign gpr_src_slvs[i].rd2 = {`RV_XLEN{chn_sels[i]}} & gpr_dst_mst.rd2;

        assign r1_addr_arr[i] = gpr_src_slvs[i].ra1;
        assign r2_addr_arr[i] = gpr_src_slvs[i].ra2;
        assign w_wen_arr[i] = gpr_src_slvs[i].wen;
        assign w_addr_arr[i] = gpr_src_slvs[i].wa;
        assign w_data_arr[i] = gpr_src_slvs[i].wd;
    end

    always_comb begin
        gpr_dst_mst.ra1 = {`RV_GPR_AW{1'b0}};
        gpr_dst_mst.ra2 = {`RV_GPR_AW{1'b0}};
        gpr_dst_mst.wen = 1'b0;
        gpr_dst_mst.wa = {`RV_GPR_AW{1'b0}};
        gpr_dst_mst.wd = {`RV_XLEN{1'b0}};

        for (int i = 0; i < CHN_NUM; i++) begin
            gpr_dst_mst.ra1 |= ({`RV_GPR_AW{chn_sels[i]}} & r1_addr_arr[i]);
            gpr_dst_mst.ra2 |= ({`RV_GPR_AW{chn_sels[i]}} & r2_addr_arr[i]);
            gpr_dst_mst.wen |= (chn_sels[i] & w_wen_arr[i]);
            gpr_dst_mst.wa |= ({`RV_GPR_AW{chn_sels[i]}} & w_addr_arr[i]);
            gpr_dst_mst.wd |= ({`RV_XLEN{chn_sels[i]}} & w_data_arr[i]);
        end
    end
endmodule