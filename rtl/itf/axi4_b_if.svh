`ifndef AXI4_B_IF_SVH
`define AXI4_B_IF_SVH

typedef enum logic [1:0] {
    AXI4_B_RESP_OKAY = 2'd0,
    AXI4_B_RESP_EXOKAY = 2'd1,
    AXI4_B_RESP_SLVERR = 2'd2,
    AXI4_B_RESP_DECERR = 2'd3
} axi4_b_resp_t;

`endif
