`include "itf/axi4_r_if.svh"
`include "itf/axi4_b_if.svh"

module axi_demux #(
    parameter GST_NUM = 5,
    parameter logic [31:0] GST_BASE[GST_NUM],
    parameter logic [31:0] GST_SIZE[GST_NUM]
)(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  host_axi4_aw_slv,
    axi4_w_if_t.slv   host_axi4_w_slv,
    axi4_b_if_t.mst   host_axi4_b_mst,
    axi4_ar_if_t.slv  host_axi4_ar_slv,
    axi4_r_if_t.mst   host_axi4_r_mst,
    axi4_aw_if_t.mst  gst_axi4_aw_msts[GST_NUM],
    axi4_w_if_t.mst   gst_axi4_w_msts[GST_NUM],
    axi4_b_if_t.slv   gst_axi4_b_slvs[GST_NUM],
    axi4_ar_if_t.mst  gst_axi4_ar_msts[GST_NUM],
    axi4_r_if_t.slv   gst_axi4_r_slvs[GST_NUM]
);
    logic [GST_NUM-1:0] rd_sel;
    logic [GST_NUM-1:0] wr_sel;
    logic [GST_NUM-1:0] rd_pending;
    logic [GST_NUM-1:0] wr_pending;
    logic rd_decerr;
    logic wr_decerr;
    logic [7:0] rd_decerr_id;
    logic [7:0] wr_decerr_id;
    logic gst_ar_rdy[GST_NUM];
    logic gst_aw_rdy[GST_NUM];
    logic gst_w_rdy[GST_NUM];
    logic gst_r_vld[GST_NUM];
    logic [7:0] gst_r_id[GST_NUM];
    logic [31:0] gst_r_data[GST_NUM];
    axi4_r_resp_t gst_r_resp[GST_NUM];
    logic gst_r_last[GST_NUM];
    logic gst_b_vld[GST_NUM];
    logic [7:0] gst_b_id[GST_NUM];
    axi4_b_resp_t gst_b_resp[GST_NUM];

    for (genvar i = 0; i < GST_NUM; i++) begin
        assign rd_sel[i] = ({1'b0, host_axi4_ar_slv.pkt.addr} -
            {1'b0, GST_BASE[i]}) < {1'b0, GST_SIZE[i]};
        assign wr_sel[i] = ({1'b0, host_axi4_aw_slv.pkt.addr} -
            {1'b0, GST_BASE[i]}) < {1'b0, GST_SIZE[i]};

        assign gst_axi4_ar_msts[i].vld = !(|rd_pending) && !rd_decerr &&
            host_axi4_ar_slv.vld && rd_sel[i];
        assign gst_axi4_ar_msts[i].pkt = host_axi4_ar_slv.pkt;
        assign gst_axi4_aw_msts[i].vld = !(|wr_pending) && !wr_decerr && host_axi4_aw_slv.vld &&
            host_axi4_w_slv.vld && wr_sel[i];
        assign gst_axi4_aw_msts[i].pkt = host_axi4_aw_slv.pkt;
        assign gst_axi4_w_msts[i].vld = !(|wr_pending) && !wr_decerr && host_axi4_aw_slv.vld &&
            host_axi4_w_slv.vld && wr_sel[i];
        assign gst_axi4_w_msts[i].pkt = host_axi4_w_slv.pkt;
        assign gst_axi4_r_slvs[i].rdy = rd_pending[i] && host_axi4_r_mst.rdy;
        assign gst_axi4_b_slvs[i].rdy = wr_pending[i] && host_axi4_b_mst.rdy;
        assign gst_ar_rdy[i] = gst_axi4_ar_msts[i].rdy;
        assign gst_aw_rdy[i] = gst_axi4_aw_msts[i].rdy;
        assign gst_w_rdy[i] = gst_axi4_w_msts[i].rdy;
        assign gst_r_vld[i] = gst_axi4_r_slvs[i].vld;
        assign gst_r_id[i] = gst_axi4_r_slvs[i].pkt.id;
        assign gst_r_data[i] = gst_axi4_r_slvs[i].pkt.data;
        assign gst_r_resp[i] = gst_axi4_r_slvs[i].pkt.resp;
        assign gst_r_last[i] = gst_axi4_r_slvs[i].pkt.last;
        assign gst_b_vld[i] = gst_axi4_b_slvs[i].vld;
        assign gst_b_id[i] = gst_axi4_b_slvs[i].pkt.id;
        assign gst_b_resp[i] = gst_axi4_b_slvs[i].pkt.resp;

        always_ff @(posedge clk or negedge rst_n) begin
            if (!rst_n) begin
                rd_pending[i] <= 1'b0;
                wr_pending[i] <= 1'b0;
            end else begin
                if (gst_axi4_ar_msts[i].vld && gst_axi4_ar_msts[i].rdy)
                    rd_pending[i] <= 1'b1;
                else if (gst_axi4_r_slvs[i].vld && gst_axi4_r_slvs[i].rdy &&
                    gst_axi4_r_slvs[i].pkt.last)
                    rd_pending[i] <= 1'b0;

                if (gst_axi4_aw_msts[i].vld && gst_axi4_aw_msts[i].rdy &&
                    gst_axi4_w_msts[i].vld && gst_axi4_w_msts[i].rdy)
                    wr_pending[i] <= 1'b1;
                else if (gst_axi4_b_slvs[i].vld && gst_axi4_b_slvs[i].rdy)
                    wr_pending[i] <= 1'b0;
            end
        end
    end

    wire rd_miss = !(|rd_sel);
    wire wr_miss = !(|wr_sel);
    wire rd_decerr_hsk = rd_decerr && host_axi4_r_mst.rdy;
    wire wr_decerr_hsk = wr_decerr && host_axi4_b_mst.rdy;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_decerr <= 1'b0;
            wr_decerr <= 1'b0;
            rd_decerr_id <= 0;
            wr_decerr_id <= 0;
        end else begin
            if (!(|rd_pending) && !rd_decerr && host_axi4_ar_slv.vld &&
                host_axi4_ar_slv.rdy && rd_miss) begin
                rd_decerr <= 1'b1;
                rd_decerr_id <= host_axi4_ar_slv.pkt.id;
            end else if (rd_decerr_hsk) begin
                rd_decerr <= 1'b0;
            end
            if (!(|wr_pending) && !wr_decerr && host_axi4_aw_slv.vld &&
                host_axi4_aw_slv.rdy && host_axi4_w_slv.vld &&
                host_axi4_w_slv.rdy && wr_miss) begin
                wr_decerr <= 1'b1;
                wr_decerr_id <= host_axi4_aw_slv.pkt.id;
            end else if (wr_decerr_hsk) begin
                wr_decerr <= 1'b0;
            end
        end
    end

    always_comb begin
        host_axi4_ar_slv.rdy = 1'b0;
        host_axi4_aw_slv.rdy = 1'b0;
        host_axi4_w_slv.rdy = 1'b0;
        host_axi4_r_mst.vld = 1'b0;
        host_axi4_r_mst.pkt.id = '0;
        host_axi4_r_mst.pkt.data = '0;
        host_axi4_r_mst.pkt.resp = AXI4_R_RESP_OKAY;
        host_axi4_r_mst.pkt.last = 1'b0;
        host_axi4_b_mst.vld = 1'b0;
        host_axi4_b_mst.pkt.id = '0;
        host_axi4_b_mst.pkt.resp = AXI4_B_RESP_OKAY;
        if (!(|rd_pending) && !rd_decerr && rd_miss) begin
            host_axi4_ar_slv.rdy = 1'b1;
        end
        if (!(|wr_pending) && !wr_decerr && wr_miss &&
            host_axi4_aw_slv.vld && host_axi4_w_slv.vld) begin
            host_axi4_aw_slv.rdy = 1'b1;
            host_axi4_w_slv.rdy = 1'b1;
        end
        if (rd_decerr) begin
            host_axi4_r_mst.vld = 1'b1;
            host_axi4_r_mst.pkt.id = rd_decerr_id;
            host_axi4_r_mst.pkt.data = 0;
            host_axi4_r_mst.pkt.resp = AXI4_R_RESP_DECERR;
            host_axi4_r_mst.pkt.last = 1'b1;
        end
        if (wr_decerr) begin
            host_axi4_b_mst.vld = 1'b1;
            host_axi4_b_mst.pkt.id = wr_decerr_id;
            host_axi4_b_mst.pkt.resp = AXI4_B_RESP_DECERR;
        end
        for (int i = 0; i < GST_NUM; i++) begin
            host_axi4_ar_slv.rdy |= gst_ar_rdy[i] && rd_sel[i] && !(|rd_pending) && !rd_decerr;
            host_axi4_aw_slv.rdy |= gst_aw_rdy[i] && gst_w_rdy[i] &&
                wr_sel[i] && !(|wr_pending) && !wr_decerr;
            host_axi4_w_slv.rdy |= gst_aw_rdy[i] && gst_w_rdy[i] &&
                wr_sel[i] && !(|wr_pending) && !wr_decerr;
            host_axi4_r_mst.vld |= gst_r_vld[i] && rd_pending[i];
            host_axi4_r_mst.pkt.id |= gst_r_id[i] & {8{rd_pending[i]}};
            host_axi4_r_mst.pkt.data |= gst_r_data[i] & {32{rd_pending[i]}};
            host_axi4_r_mst.pkt.resp = rd_pending[i] ? gst_r_resp[i] : host_axi4_r_mst.pkt.resp;
            host_axi4_r_mst.pkt.last |= gst_r_last[i] && rd_pending[i];
            host_axi4_b_mst.vld |= gst_b_vld[i] && wr_pending[i];
            host_axi4_b_mst.pkt.id |= gst_b_id[i] & {8{wr_pending[i]}};
            host_axi4_b_mst.pkt.resp = wr_pending[i] ? gst_b_resp[i] : host_axi4_b_mst.pkt.resp;
        end
    end
endmodule
