interface fl_req_if_t;
    logic vld;

    modport mst(output vld);
    modport slv(input vld);
endinterface
