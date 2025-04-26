`include "core/isa.svh"

interface ex_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        rv32i_inst_t            ir;
        logic [`RV_PC_SIZE-1:0] pc;
        logic                   pred_taken;
        logic [`RV_PC_SIZE-1:0] pred_pc;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface