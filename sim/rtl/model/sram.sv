module sram #(
    parameter AW = 15,
    parameter DW = 32
)(
    input            clk,
    sram_rw_if_t.slv sram_rw_slv
);
    logic [DW-1:0] mem[0:2**AW-1];
    always @(posedge clk) begin
        if (sram_rw_slv.cs & sram_rw_slv.wen)
            mem[sram_rw_slv.addr] <= sram_rw_slv.wdata;
    end

    always @(posedge clk) begin
        if (sram_rw_slv.cs & ~sram_rw_slv.wen) begin
            sram_rw_slv.rdata <= mem[sram_rw_slv.addr];
        end
    end
endmodule