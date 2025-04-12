module rom #(
    parameter AW = 15,
    parameter DW = 32
)(
    input logic           clk,
    input logic           cs,
    input logic  [AW-1:0] addr,
    output logic [DW-1:0] data
);
    logic [DW-1:0] mem[0:2**AW-1];
    always @(posedge clk) begin
        if (cs)
            data <= mem[addr];
    end
endmodule
