`ifndef SDSPI_CMD_IF_SVH
`define SDSPI_CMD_IF_SVH

typedef enum logic [1:0] {
    SDSPI_CMD_KIND_COMMAND = 2'd0,
    SDSPI_CMD_KIND_WRITE_DATA = 2'd1,
    SDSPI_CMD_KIND_RESET = 2'd2,
    SDSPI_CMD_KIND_CONFIG = 2'd3
} sdspi_cmd_kind_t;

`endif
