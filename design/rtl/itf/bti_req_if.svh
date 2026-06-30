`ifndef BTI_REQ_IF_SVH
`define BTI_REQ_IF_SVH

typedef enum logic [0:0] {
    BTI_REQ_CMD_READ = 1'd0,
    BTI_REQ_CMD_WRITE = 1'd1
} bti_req_cmd_t;

`endif
