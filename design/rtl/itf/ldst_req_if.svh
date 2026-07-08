`ifndef LDST_REQ_IF_SVH
`define LDST_REQ_IF_SVH

typedef enum logic [1:0] {
    LDST_REQ_CMO_NONE = 2'd0,
    LDST_REQ_CMO_INVAL = 2'd1,
    LDST_REQ_CMO_CLEAN = 2'd2,
    LDST_REQ_CMO_FLUSH = 2'd3
} ldst_req_cmo_t;

typedef enum logic [1:0] {
    LDST_REQ_SIZE_B1 = 2'd0,
    LDST_REQ_SIZE_B2 = 2'd1,
    LDST_REQ_SIZE_B4 = 2'd2
} ldst_req_size_t;

`endif
