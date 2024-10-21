module rv32i #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,
    sram_if.master    sram_rw
);

    ifetch_if ifetch();
    iexec_if iexec();

    ifu u_ifu(.*);
    exu u_exu(.*);
    biu u_biu(.*);

endmodule