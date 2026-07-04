module axi_link(
    axi4_aw_if_t.slv host_axi4_aw_slv,
    axi4_w_if_t.slv  host_axi4_w_slv,
    axi4_b_if_t.mst  host_axi4_b_mst,
    axi4_ar_if_t.slv host_axi4_ar_slv,
    axi4_r_if_t.mst  host_axi4_r_mst,
    axi4_aw_if_t.mst gst_axi4_aw_mst,
    axi4_w_if_t.mst  gst_axi4_w_mst,
    axi4_b_if_t.slv  gst_axi4_b_slv,
    axi4_ar_if_t.mst gst_axi4_ar_mst,
    axi4_r_if_t.slv  gst_axi4_r_slv
);
    assign gst_axi4_aw_mst.vld = host_axi4_aw_slv.vld;
    assign gst_axi4_aw_mst.pkt = host_axi4_aw_slv.pkt;
    assign host_axi4_aw_slv.rdy = gst_axi4_aw_mst.rdy;
    assign gst_axi4_w_mst.vld = host_axi4_w_slv.vld;
    assign gst_axi4_w_mst.pkt = host_axi4_w_slv.pkt;
    assign host_axi4_w_slv.rdy = gst_axi4_w_mst.rdy;
    assign host_axi4_b_mst.vld = gst_axi4_b_slv.vld;
    assign host_axi4_b_mst.pkt = gst_axi4_b_slv.pkt;
    assign gst_axi4_b_slv.rdy = host_axi4_b_mst.rdy;
    assign gst_axi4_ar_mst.vld = host_axi4_ar_slv.vld;
    assign gst_axi4_ar_mst.pkt = host_axi4_ar_slv.pkt;
    assign host_axi4_ar_slv.rdy = gst_axi4_ar_mst.rdy;
    assign host_axi4_r_mst.vld = gst_axi4_r_slv.vld;
    assign host_axi4_r_mst.pkt = gst_axi4_r_slv.pkt;
    assign gst_axi4_r_slv.rdy = host_axi4_r_mst.rdy;
endmodule
