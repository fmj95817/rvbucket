`ifndef BUS_SVH
`define BUS_SVH

typedef enum logic {
    BUS_CMD_READ = 1'b0,
    BUS_CMD_WRITE = 1'b1
} bus_cmd_t;

`endif