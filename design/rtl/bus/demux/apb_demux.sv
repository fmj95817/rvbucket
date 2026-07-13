module apb_demux #(
    parameter int GST_NUM = 4,
    parameter logic [31:0] GST_BASE[GST_NUM],
    parameter logic [31:0] GST_SIZE[GST_NUM]
)(
    apb_req_if_t.slv host_apb_req_slv,
    apb_rsp_if_t.mst host_apb_rsp_mst,
    apb_req_if_t.mst gst_apb_req_msts[GST_NUM],
    apb_rsp_if_t.slv gst_apb_rsp_slvs[GST_NUM]
);
    logic [GST_NUM-1:0] sel;
    logic [GST_NUM-1:0] gst_pready;
    logic [31:0] gst_prdata[GST_NUM];
    logic [GST_NUM-1:0] gst_pslverr;

    for (genvar i = 0; i < GST_NUM; i++) begin
        assign sel[i] = ({1'b0, host_apb_req_slv.pkt.paddr} -
            {1'b0, GST_BASE[i]}) < {1'b0, GST_SIZE[i]};
        assign gst_apb_req_msts[i].psel = host_apb_req_slv.psel && sel[i];
        assign gst_apb_req_msts[i].penable = host_apb_req_slv.penable && sel[i];
        assign gst_apb_req_msts[i].pkt = host_apb_req_slv.pkt;
        assign gst_pready[i] = gst_apb_rsp_slvs[i].pready;
        assign gst_prdata[i] = gst_apb_rsp_slvs[i].pkt.prdata;
        assign gst_pslverr[i] = gst_apb_rsp_slvs[i].pkt.pslverr;
    end

    always_comb begin
        host_apb_rsp_mst.pready = 1'b0;
        host_apb_rsp_mst.pkt.prdata = '0;
        host_apb_rsp_mst.pkt.pslverr = 1'b0;
        if (host_apb_req_slv.psel && host_apb_req_slv.penable && !(|sel)) begin
            host_apb_rsp_mst.pready = 1'b1;
            host_apb_rsp_mst.pkt.pslverr = 1'b1;
        end
        for (int i = 0; i < GST_NUM; i++) begin
            if (sel[i]) begin
                host_apb_rsp_mst.pready |= gst_pready[i];
                host_apb_rsp_mst.pkt.prdata |= gst_prdata[i];
                host_apb_rsp_mst.pkt.pslverr |= gst_pslverr[i];
            end
        end
    end
endmodule
