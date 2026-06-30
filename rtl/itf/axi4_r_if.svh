`ifndef AXI4_R_IF_SVH
`define AXI4_R_IF_SVH

typedef enum logic [1:0] {
    AXI4_R_RESP_OKAY = 2'd0,
    AXI4_R_RESP_EXOKAY = 2'd1,
    AXI4_R_RESP_SLVERR = 2'd2,
    AXI4_R_RESP_DECERR = 2'd3
} axi4_r_resp_t;

`endif
