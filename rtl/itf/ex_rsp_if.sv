`include "core/isa.svh"

interface ex_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic taken;
        logic pred_true;
        logic [`RV_PC_SIZE-1:0] pc;
        logic [`RV_PC_SIZE-1:0] target_pc;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface