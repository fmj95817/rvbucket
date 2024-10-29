`include "isa.svh"

interface ifetch_if;
    logic req_vld;
    logic req_rdy;
    logic [`RV_PC_SIZE-1:0] req_pc;

    logic rsp_vld;
    logic rsp_rdy;
    logic [`RV_IR_SIZE-1:0] rsp_ir;

    modport master (
        output req_vld, req_pc, input req_rdy,
        input rsp_vld, rsp_ir, output rsp_rdy
    );

    modport slave (
        input req_vld, req_pc, output req_rdy,
        output rsp_vld, rsp_ir, input rsp_rdy
    );
endinterface

interface iexec_if;
    logic req_vld;
    logic req_rdy;
    struct packed {
        logic [`RV_IR_SIZE-1:0] ir;
        logic [`RV_PC_SIZE-1:0] pc;
        logic valid;
    } req_pkt;

    struct packed {
        logic taken;
        logic [`RV_PC_SIZE-1:0] offset;
    } rsp_pkt;

    modport master (
        output req_vld, req_pkt, input req_rdy,
        input rsp_pkt
    );

    modport slave (
        input req_vld, req_pkt, output req_rdy,
        output rsp_pkt
    );
endinterface

interface ldst_if;
    logic req_vld;
    logic req_rdy;
    struct packed {
        logic [`RV_AW-1:0]     addr;
        logic                  st;
        logic [`RV_XLEN-1:0]   data;
        logic [`RV_XLEN/8-1:0] strobe;
    } req_pkt;

    logic rsp_vld;
    logic rsp_rdy;
    struct packed {
        logic [`RV_XLEN-1:0] data;
        logic                ok;
    } rsp_pkt;

    modport master (
        output req_vld, req_pkt, input req_rdy,
        input rsp_vld, rsp_pkt, output rsp_rdy
    );

    modport slave (
        input req_vld, req_pkt, output req_rdy,
        output rsp_vld, rsp_pkt, input rsp_rdy
    );
endinterface