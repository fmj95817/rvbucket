module rv32i #(
    parameter AW = 32,
    parameter DW = 32,
    parameter SRAM_AW = 15
)(
    input             clk,
    input             rst_n,
    sram_if_t.master  sram_rw
);

    ifetch_if_t ifetch();
    iexec_if_t iexec();

    ifu u_ifu(.*);
    exu u_exu(.*);
    biu u_biu(.*);

endmodule