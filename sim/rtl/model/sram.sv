module sram #(
    parameter AW = 15,
    parameter DW = 32
)(
    input logic      clk,
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

module dp_sram #(
    parameter AW = 15,
    parameter DW = 32
)(
    input logic      clk,
    sram_rw_if_t.slv sram_r_slv,
    sram_rw_if_t.slv sram_w_slv
);
    assign sram_w_slv.rdata = {DW{1'b0}};

    logic [DW-1:0] mem[0:2**AW-1];
    always @(posedge clk) begin
        if (sram_w_slv.cs & sram_w_slv.wen)
            mem[sram_w_slv.addr] <= sram_w_slv.wdata;
    end

    always @(posedge clk) begin
        if (sram_r_slv.cs & ~sram_r_slv.wen) begin
            sram_r_slv.rdata <= mem[sram_r_slv.addr];
        end
    end

endmodule