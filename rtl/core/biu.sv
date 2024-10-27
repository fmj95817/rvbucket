`include "bus.svh"

module biu(
    input               clk,
    input               rst_n,
    ifetch_if.slave     ifetch,
    ldst_if.slave       ldst_gen,
    bus_trans_if.master bti
);
    assign bti.req_vld = ifetch.req_vld;
    assign bti.req_pkt.cmd = BUS_CMD_READ;
    assign bti.req_pkt.addr = ifetch.req_pc;
    assign ifetch.req_rdy = bti.req_rdy;

    assign ifetch.rsp_vld = bti.rsp_vld;
    assign ifetch.rsp_ir = bti.rsp_pkt.data;
    assign bti.rsp_rdy = ifetch.rsp_rdy;
endmodule