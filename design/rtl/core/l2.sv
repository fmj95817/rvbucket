module l2(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  i_axi4_aw_slv,
    axi4_w_if_t.slv   i_axi4_w_slv,
    axi4_b_if_t.mst   i_axi4_b_mst,
    axi4_ar_if_t.slv  i_axi4_ar_slv,
    axi4_r_if_t.mst   i_axi4_r_mst,
    axi4_aw_if_t.slv  d_axi4_aw_slv,
    axi4_w_if_t.slv   d_axi4_w_slv,
    axi4_b_if_t.mst   d_axi4_b_mst,
    axi4_ar_if_t.slv  d_axi4_ar_slv,
    axi4_r_if_t.mst   d_axi4_r_mst,
    axi4_aw_if_t.mst  mem_axi4_aw_mst,
    axi4_w_if_t.mst   mem_axi4_w_mst,
    axi4_b_if_t.slv   mem_axi4_b_slv,
    axi4_ar_if_t.mst  mem_axi4_ar_mst,
    axi4_r_if_t.slv   mem_axi4_r_slv
);
    logic rd_active;
    logic rd_active_d;
    logic rd_rr_d;
    logic wr_active;
    logic wr_active_d;
    logic wr_rr_d;

    wire rd_select_d = d_axi4_ar_slv.vld && (rd_rr_d || !i_axi4_ar_slv.vld);
    wire rd_select_i = i_axi4_ar_slv.vld && !rd_select_d;
    wire rd_req_hsk = mem_axi4_ar_mst.vld && mem_axi4_ar_mst.rdy;
    wire rd_rsp_hsk = mem_axi4_r_slv.vld && mem_axi4_r_slv.rdy && mem_axi4_r_slv.pkt.last;

    wire i_write_req = i_axi4_aw_slv.vld && i_axi4_w_slv.vld;
    wire d_write_req = d_axi4_aw_slv.vld && d_axi4_w_slv.vld;
    wire wr_select_d = d_write_req && (wr_rr_d || !i_write_req);
    wire wr_select_i = i_write_req && !wr_select_d;
    wire wr_req_hsk = mem_axi4_aw_mst.vld && mem_axi4_aw_mst.rdy &&
        mem_axi4_w_mst.vld && mem_axi4_w_mst.rdy;
    wire wr_rsp_hsk = mem_axi4_b_slv.vld && mem_axi4_b_slv.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_active <= 1'b0;
            rd_active_d <= 1'b0;
            rd_rr_d <= 1'b0;
            wr_active <= 1'b0;
            wr_active_d <= 1'b0;
            wr_rr_d <= 1'b0;
        end else begin
            if (!rd_active && rd_req_hsk) begin
                rd_active <= 1'b1;
                rd_active_d <= rd_select_d;
                rd_rr_d <= !rd_select_d;
            end else if (rd_active && rd_rsp_hsk) begin
                rd_active <= 1'b0;
            end

            if (!wr_active && wr_req_hsk) begin
                wr_active <= 1'b1;
                wr_active_d <= wr_select_d;
                wr_rr_d <= !wr_select_d;
            end else if (wr_active && wr_rsp_hsk) begin
                wr_active <= 1'b0;
            end
        end
    end

    assign mem_axi4_ar_mst.vld = !rd_active && (rd_select_i || rd_select_d);
    assign mem_axi4_ar_mst.pkt = rd_select_d ? d_axi4_ar_slv.pkt : i_axi4_ar_slv.pkt;
    assign i_axi4_ar_slv.rdy = !rd_active && rd_select_i && mem_axi4_ar_mst.rdy;
    assign d_axi4_ar_slv.rdy = !rd_active && rd_select_d && mem_axi4_ar_mst.rdy;
    assign i_axi4_r_mst.vld = rd_active && !rd_active_d && mem_axi4_r_slv.vld;
    assign i_axi4_r_mst.pkt = mem_axi4_r_slv.pkt;
    assign d_axi4_r_mst.vld = rd_active && rd_active_d && mem_axi4_r_slv.vld;
    assign d_axi4_r_mst.pkt = mem_axi4_r_slv.pkt;
    assign mem_axi4_r_slv.rdy = rd_active_d ? d_axi4_r_mst.rdy : i_axi4_r_mst.rdy;

    assign mem_axi4_aw_mst.vld = !wr_active && (wr_select_i || wr_select_d);
    assign mem_axi4_aw_mst.pkt = wr_select_d ? d_axi4_aw_slv.pkt : i_axi4_aw_slv.pkt;
    assign mem_axi4_w_mst.vld = !wr_active && (wr_select_i || wr_select_d);
    assign mem_axi4_w_mst.pkt = wr_select_d ? d_axi4_w_slv.pkt : i_axi4_w_slv.pkt;
    assign i_axi4_aw_slv.rdy = !wr_active && wr_select_i && mem_axi4_aw_mst.rdy && mem_axi4_w_mst.rdy;
    assign i_axi4_w_slv.rdy = i_axi4_aw_slv.rdy;
    assign d_axi4_aw_slv.rdy = !wr_active && wr_select_d && mem_axi4_aw_mst.rdy && mem_axi4_w_mst.rdy;
    assign d_axi4_w_slv.rdy = d_axi4_aw_slv.rdy;
    assign i_axi4_b_mst.vld = wr_active && !wr_active_d && mem_axi4_b_slv.vld;
    assign i_axi4_b_mst.pkt = mem_axi4_b_slv.pkt;
    assign d_axi4_b_mst.vld = wr_active && wr_active_d && mem_axi4_b_slv.vld;
    assign d_axi4_b_mst.pkt = mem_axi4_b_slv.pkt;
    assign mem_axi4_b_slv.rdy = wr_active_d ? d_axi4_b_mst.rdy : i_axi4_b_mst.rdy;
endmodule
