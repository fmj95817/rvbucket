interface ex_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [31:0] pc;
        logic taken;
        logic pred_true;
        logic [31:0] target_pc;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
