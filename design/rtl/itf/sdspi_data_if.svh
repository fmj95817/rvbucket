`ifndef SDSPI_DATA_IF_SVH
`define SDSPI_DATA_IF_SVH

typedef enum logic [1:0] {
    SDSPI_DATA_KIND_STATUS = 2'd0,
    SDSPI_DATA_KIND_RESPONSE = 2'd1,
    SDSPI_DATA_KIND_READ_DATA = 2'd2,
    SDSPI_DATA_KIND_WRITE_DONE = 2'd3
} sdspi_data_kind_t;

`endif
