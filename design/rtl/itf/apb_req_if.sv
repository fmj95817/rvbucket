interface apb_req_if_t;
    logic psel;
    logic penable;

    struct packed {
        logic [31:0] paddr;
        logic pwrite;
        logic [31:0] pwdata;
        logic [3:0] pstrb;
    } pkt;

    modport mst (output pkt, psel, penable);
    modport slv (input pkt, psel, penable);
endinterface
