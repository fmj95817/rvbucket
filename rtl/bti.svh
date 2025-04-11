`ifndef BTI_SVH
`define BTI_SVH

`define BTI_TIDW 4

typedef enum logic {
    BTI_CMD_READ = 1'b0,
    BTI_CMD_WRITE = 1'b1
} bti_cmd_t;

`endif