`ifndef CACHE_SVH
`define CACHE_SVH

`define AXI4_USER_CMO_OP_NONE  2'd0
`define AXI4_USER_CMO_OP_INVAL 2'd1
`define AXI4_USER_CMO_OP_CLEAN 2'd2
`define AXI4_USER_CMO_OP_FLUSH 2'd3
`define AXI4_USER_CMO_TAG       6'h30

`define AXI4_USER_MAKE_CMO(op) \
    {24'b0, `AXI4_USER_CMO_TAG, (op)}

`define AXI4_USER_IS_CMO(user) \
    ((user[31:8]) == 24'b0 && (user[7:2]) == `AXI4_USER_CMO_TAG && \
        (user[1:0]) != `AXI4_USER_CMO_OP_NONE)

`define AXI4_USER_CMO_CLEANS(user) \
    ((user[1:0]) == `AXI4_USER_CMO_OP_CLEAN || \
        (user[1:0]) == `AXI4_USER_CMO_OP_FLUSH)

`define AXI4_USER_CMO_INVALIDATES(user) \
    ((user[1:0]) == `AXI4_USER_CMO_OP_INVAL || \
        (user[1:0]) == `AXI4_USER_CMO_OP_FLUSH)

`endif
