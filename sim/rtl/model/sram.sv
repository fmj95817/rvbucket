module sram #(
    parameter AW = 15,
    parameter DW = 32
)(
    input         clk,
    sram_if.slave sram_rw
);
    logic [DW-1:0] mem[0:2**AW-1];
    always @(posedge clk) begin
        if (sram_rw.wen)
            mem[sram_rw.addr] <= #1 sram_rw.wdata;
    end

    always @(posedge clk) begin
        if (~sram_rw.wen) begin
            sram_rw.rdata <= #1 mem[sram_rw.addr];
        end
    end
endmodule