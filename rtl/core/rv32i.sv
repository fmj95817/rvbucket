module rv32i #(
    parameter AW = 32,
    parameter DW = 32,
    parameter SRAM_AW = 15
)(
    input                 clk,
    input                 rst_n,
    bus_trans_if_t.master bti
);
    ifetch_if_t ifetch();
    iexec_if_t iexec();
    ldst_if_t ldst_src();
    ldst_if_t ldst_gen();

    ifu u_ifu(.*);
    exu u_exu(.*);
    lsu u_lsu(.*);
    biu u_biu(.*);
endmodule