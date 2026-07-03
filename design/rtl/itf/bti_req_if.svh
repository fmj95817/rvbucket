`ifndef BTI_REQ_IF_SVH
`define BTI_REQ_IF_SVH

typedef enum logic [0:0] {
    BTI_REQ_CMD_READ = 1'd0,
    BTI_REQ_CMD_WRITE = 1'd1
} bti_req_cmd_t;

typedef enum logic [1:0] {
    BTI_REQ_SIZE_B1 = 2'd0,
    BTI_REQ_SIZE_B2 = 2'd1,
    BTI_REQ_SIZE_B4 = 2'd2
} bti_req_size_t;

`endif
