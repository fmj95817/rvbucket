module bti_link(
    bti_req_if_t.slv host_bti_req_slv,
    bti_rsp_if_t.mst host_bti_rsp_mst,
    bti_req_if_t.mst gst_bti_req_mst,
    bti_rsp_if_t.slv gst_bti_rsp_slv
);
    assign gst_bti_req_mst.vld = host_bti_req_slv.vld;
    assign gst_bti_req_mst.pkt = host_bti_req_slv.pkt;
    assign host_bti_req_slv.rdy = gst_bti_req_mst.rdy;

    assign host_bti_rsp_mst.vld = gst_bti_rsp_slv.vld;
    assign host_bti_rsp_mst.pkt = gst_bti_rsp_slv.pkt;
    assign gst_bti_rsp_slv.rdy = host_bti_rsp_mst.rdy;
endmodule
