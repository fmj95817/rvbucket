module sram #(
    parameter AW = 10,
    parameter DW = 32
)(
    input  tri             clk,

    input  tri   [AW-1:0]  addr,
    output logic [DW-1:0]  rdata,
    input  tri   [DW-1:0]  wdata,
    input  tri             wen
);
    reg [DW-1:0] mem[0:2**AW-1];
    always @(posedge clk) begin
        if (wen)
            mem[addr] <= wdata;
    end

    always @(posedge clk) begin
        if (~wen) begin
            rdata <= mem[addr];
        end
    end

    initial begin
        $readmemh("data.hex", mem);
    end
endmodule