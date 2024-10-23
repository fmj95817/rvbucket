module lib_flag(
    input tri clk,
    input tri rst_n,
    input tri set_cond,
    input tri clear_cond,
    output logic flag
);
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            flag <= 1'b0;
        else if (set_cond | clear_cond)
            flag <= #1 set_cond | (~clear_cond);
    end
endmodule