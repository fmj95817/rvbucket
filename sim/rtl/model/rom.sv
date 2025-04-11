module rom #(
    parameter AW = 15,
    parameter DW = 32
)(
    input tri             clk,
    input tri    [AW-1:0] rom_addr,
    output logic [DW-1:0] rom_data
);
    logic [DW-1:0] data[0:2**AW-1];
    always @(posedge clk) begin
        rom_data <= data[rom_addr];
    end
endmodule
