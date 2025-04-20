module uart(
    input  logic       clk,
    input  logic       rst_n,
    input  logic       ch_vld,
    input  logic [7:0] ch,
    output logic       done
);
    always @(posedge clk) begin
        if (ch_vld) begin
            if (ch == 8'h10)
                $finish;
            $write("%c", ch);
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            done <= 1'b0;
        else if (ch_vld)
            done <= 1'b1;
        else if (done)
            done <= 1'b0;
    end

endmodule