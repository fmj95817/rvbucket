`ifndef AXI4_AW_IF_SVH
`define AXI4_AW_IF_SVH

typedef enum logic [1:0] {
    AXI4_AW_BURST_FIXED = 2'd0,
    AXI4_AW_BURST_INCR = 2'd1,
    AXI4_AW_BURST_WRAP = 2'd2
} axi4_aw_burst_t;

typedef enum logic [2:0] {
    AXI4_AW_SIZE_B1 = 3'd0,
    AXI4_AW_SIZE_B2 = 3'd1,
    AXI4_AW_SIZE_B4 = 3'd2,
    AXI4_AW_SIZE_B8 = 3'd3,
    AXI4_AW_SIZE_B16 = 3'd4,
    AXI4_AW_SIZE_B32 = 3'd5,
    AXI4_AW_SIZE_B64 = 3'd6,
    AXI4_AW_SIZE_B128 = 3'd7
} axi4_aw_size_t;

`endif
