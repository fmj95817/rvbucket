`include "bti.svh"

module biu(
    input logic       clk,
    input logic       rst_n,
    fch_req_if_t.slv  fch_req_slv,
    fch_rsp_if_t.mst  fch_rsp_mst,
    ldst_req_if_t.slv ldst_req_slv,
    ldst_rsp_if_t.mst ldst_rsp_mst,
    bti_req_if_t.mst  itcm_bti_req_mst,
    bti_rsp_if_t.slv  itcm_bti_rsp_slv,
    bti_req_if_t.mst  dtcm_bti_req_mst,
    bti_rsp_if_t.slv  dtcm_bti_rsp_slv
);

    assign itcm_bti_req_mst.vld = fch_req_slv.vld;
    assign itcm_bti_req_mst.pkt.tid = {`BTI_TIDW{1'b0}};
    assign itcm_bti_req_mst.pkt.cmd = BTI_CMD_READ;
    assign itcm_bti_req_mst.pkt.addr = fch_req_slv.pkt.pc;
    assign fch_req_slv.rdy = itcm_bti_req_mst.rdy;

    assign fch_rsp_mst.vld = itcm_bti_rsp_slv.vld;
    assign fch_rsp_mst.pkt.ir = itcm_bti_rsp_slv.pkt.data;
    assign itcm_bti_rsp_slv.rdy = fch_rsp_mst.rdy;

    assign dtcm_bti_req_mst.vld = ldst_req_slv.vld;
    assign dtcm_bti_req_mst.pkt.tid = {`BTI_TIDW{1'b0}};
    assign dtcm_bti_req_mst.pkt.cmd = ldst_req_slv.pkt.st ? BTI_CMD_WRITE : BTI_CMD_READ;
    assign dtcm_bti_req_mst.pkt.addr = ldst_req_slv.pkt.addr;
    assign dtcm_bti_req_mst.pkt.data = ldst_req_slv.pkt.data;
    assign dtcm_bti_req_mst.pkt.strobe = ldst_req_slv.pkt.strobe;
    assign ldst_req_slv.rdy = dtcm_bti_req_mst.rdy;

    assign ldst_rsp_mst.vld = dtcm_bti_rsp_slv.vld;
    assign ldst_rsp_mst.pkt.data = dtcm_bti_rsp_slv.pkt.data;
    assign ldst_rsp_mst.pkt.ok = dtcm_bti_rsp_slv.pkt.ok;

endmodule
