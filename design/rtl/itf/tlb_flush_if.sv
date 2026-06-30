interface tlb_flush_if_t;
    logic vld;

    modport mst (output vld);
    modport slv (input vld);
endinterface
