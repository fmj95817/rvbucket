module oldest_select #(
    parameter int DEPTH = 2,
    localparam int SLOT_W = $clog2(DEPTH)
)(
    input  logic [DEPTH-1:0]       candidate_vld,
    input  logic [DEPTH*DEPTH-1:0] older_flat,
    output logic                   select_vld,
    output logic [SLOT_W-1:0]      select_slot
);
    logic [DEPTH-1:0] head_candidate;

`ifndef SYNTHESIS
    initial begin
        assert (DEPTH >= 2)
            else $fatal(1, "oldest_select DEPTH must be >= 2");
        assert ((DEPTH & (DEPTH - 1)) == 0)
            else $fatal(1, "oldest_select DEPTH must be power-of-two");
    end
`endif

    for (genvar i = 0; i < DEPTH; i++) begin : gen_head
        logic [DEPTH-1:0] older_candidate;

        for (genvar j = 0; j < DEPTH; j++) begin : gen_older
            assign older_candidate[j] = candidate_vld[j] &&
                older_flat[j * DEPTH + i];
        end

        assign head_candidate[i] = candidate_vld[i] &&
            !(|older_candidate);
    end

    always_comb begin
        select_vld = 1'b0;
        select_slot = '0;
        for (int i = 0; i < DEPTH; i++) begin
            if (!select_vld && head_candidate[i]) begin
                select_vld = 1'b1;
                select_slot = SLOT_W'(i);
            end
        end
    end

endmodule
