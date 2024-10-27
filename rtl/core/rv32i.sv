module rv32i(
    input               clk,
    input               rst_n,
    bus_trans_if.master bti
);
    ifetch_if ifetch();
    iexec_if iexec();
    ldst_if ldst_src();
    ldst_if ldst_gen();

    ifu u_ifu(.*);
    exu u_exu(.*);
    lsu u_lsu(.*);
    biu u_biu(.*);
endmodule