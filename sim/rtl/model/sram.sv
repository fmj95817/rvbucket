module sram #(
    parameter AW = 10,
    parameter DW = 32
)(
    input  tri             clk,
    sram_if.slave          sram_rw,
);
    reg [DW-1:0] mem[0:2**AW-1];
    always @(posedge clk) begin
        if (sram_rw.wen)
            mem[sram_rw.addr] <= sram_rw.wdata;
    end

    always @(posedge clk) begin
        if (~wen) begin
            sram_rw.rdata <= mem[addr];
        end
    end

    initial begin
        $readmemh("data.hex", mem);
    end
endmodule