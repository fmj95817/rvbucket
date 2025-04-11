`include "isa.svh"

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
