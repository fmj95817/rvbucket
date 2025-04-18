`include "bti.svh"


interface bti_req_if_t #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32
);
    logic vld;
    logic rdy;

    struct packed {
        logic [`BTI_TIDW-1:0]  tid;
        bti_cmd_t              cmd;
        logic [BTI_AW-1:0]     addr;
        logic [BTI_DW-1:0]     data;
        logic [BTI_DW/8-1:0]   strobe;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface

interface bti_rsp_if_t #(
    parameter BTI_DW = 32
);
    logic vld;
    logic rdy;
    struct packed {
        logic [`BTI_TIDW-1:0] tid;
        logic [BTI_DW-1:0]    data;
        logic                 ok;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface

module bti_demux #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter GST_SEL_AW = 8,
    parameter GST_NUM = 4,
    parameter logic [GST_SEL_AW-1:0] GST_SEL_ADDRS[GST_NUM],
    parameter int GST_VLD_AW[GST_NUM]
)(
    input logic      clk,
    input logic      rst_n,
    bti_req_if_t.slv host_bti_req_slv,
    bti_rsp_if_t.mst host_bti_rsp_mst,
    bti_req_if_t.mst gst_bti_req_msts[GST_NUM],
    bti_rsp_if_t.slv gst_bti_rsp_slvs[GST_NUM]
);
    localparam GST_ADDR_END_BIT = BTI_AW - GST_SEL_AW;
    tri [BTI_AW-1:0] req_addr = host_bti_req_slv.pkt.addr;
    tri [GST_SEL_AW-1:0] sel_addr = req_addr[BTI_AW-1:GST_ADDR_END_BIT];

    logic gsts_sel[GST_NUM];
    for (genvar i = 0; i < GST_NUM; i++) begin
        assign gsts_sel[i] = (sel_addr == GST_SEL_ADDRS[i]) &&
            (req_addr[GST_ADDR_END_BIT-1:GST_VLD_AW[i]] ==
            {(GST_ADDR_END_BIT-GST_VLD_AW[i]){1'b0}});
    end

    logic [GST_NUM-1:0] gsts_req_rdy;
    logic gsts_req_hsk[GST_NUM];
    logic gsts_rsp_hsk[GST_NUM];
    for (genvar i = 0; i < GST_NUM; i++) begin
        assign gsts_req_rdy[i] = gst_bti_req_msts[i].rdy;
        assign gsts_req_hsk[i] = gst_bti_req_msts[i].vld & gst_bti_req_msts[i].rdy;
        assign gsts_rsp_hsk[i] = gst_bti_rsp_slvs[i].vld & gst_bti_rsp_slvs[i].rdy;
    end

    logic [GST_NUM-1:0] gsts_req_pend;
    tri req_pend = |gsts_req_pend;
    for (genvar i = 0; i < GST_NUM; i++) begin
        always_ff @(posedge clk or negedge rst_n) begin
            if (~rst_n)
                gsts_req_pend[i] <= 1'b0;
            else if (gsts_req_hsk[i])
                gsts_req_pend[i] <= 1'b1;
            else if (gsts_rsp_hsk[i])
                gsts_req_pend[i] <= 1'b0;
        end
    end

    assign host_bti_req_slv.rdy = (|gsts_req_rdy) & (~req_pend);
    for (genvar i = 0; i < GST_NUM; i++) begin
        assign gst_bti_req_msts[i].vld = host_bti_req_slv.vld & gsts_sel[i] & (~req_pend);
        assign gst_bti_req_msts[i].pkt = host_bti_req_slv.pkt;
    end

    logic gsts_bti_rsp_vld[GST_NUM];
    logic gsts_bti_rsp_ok[GST_NUM];
    logic [`BTI_TIDW-1:0] gsts_bti_rsp_tid[GST_NUM];
    logic [BTI_DW-1:0] gsts_bti_rsp_data[GST_NUM];
    for (genvar i = 0; i < GST_NUM; i++) begin
        assign gsts_bti_rsp_vld[i] = gst_bti_rsp_slvs[i].vld;
        assign gsts_bti_rsp_ok[i] = gst_bti_rsp_slvs[i].pkt.ok;
        assign gsts_bti_rsp_tid[i] = gst_bti_rsp_slvs[i].pkt.tid;
        assign gsts_bti_rsp_data[i] = gst_bti_rsp_slvs[i].pkt.data;
    end

    always_comb begin
        host_bti_rsp_mst.vld = 1'b0;
        host_bti_rsp_mst.pkt.tid = {`BTI_TIDW{1'b0}};
        host_bti_rsp_mst.pkt.data = {BTI_DW{1'b0}};
        host_bti_rsp_mst.pkt.ok = 1'b0;

        for (int i = 0; i < GST_NUM; i++) begin
            host_bti_rsp_mst.vld |= (gsts_bti_rsp_vld[i] & gsts_req_pend[i]);
            host_bti_rsp_mst.pkt.ok |= (gsts_bti_rsp_ok[i] & gsts_req_pend[i]);
            host_bti_rsp_mst.pkt.tid |= (gsts_bti_rsp_tid[i] & {`BTI_TIDW{gsts_req_pend[i]}});
            host_bti_rsp_mst.pkt.data |= (gsts_bti_rsp_data[i] & {BTI_DW{gsts_req_pend[i]}});
        end
    end

    for (genvar i = 0; i < GST_NUM; i++) begin
        assign gst_bti_rsp_slvs[i].rdy = host_bti_rsp_mst.rdy;
    end

endmodule
