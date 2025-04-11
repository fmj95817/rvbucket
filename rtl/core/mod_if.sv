`include "isa.svh"

interface fch_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_PC_SIZE-1:0] pc;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface

interface fch_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_IR_SIZE-1:0] ir;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface

interface ex_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        rv32i_inst_t            ir;
        logic [`RV_PC_SIZE-1:0] pc;
        logic                   valid;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface

interface ex_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic taken;
        logic [`RV_PC_SIZE-1:0] target_pc;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface

interface ldst_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_AW-1:0]     addr;
        logic                  st;
        logic [`RV_XLEN-1:0]   data;
        logic [`RV_XLEN/8-1:0] strobe;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface


interface ldst_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_XLEN-1:0] data;
        logic                ok;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
