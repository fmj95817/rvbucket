module sram #(
    parameter AW = 10,
    parameter DW = 32
)(
    input            clk,
    sram_if_t.slave  sram_rw
);
    reg [DW-1:0] mem[0:2**AW-1];
    always @(posedge clk) begin
        if (sram_rw.wen)
            mem[sram_rw.addr] <= sram_rw.wdata;
    end

    always @(posedge clk) begin
        if (~sram_rw.wen) begin
            sram_rw.rdata <= mem[sram_rw.addr];
        end
    end
endmodule