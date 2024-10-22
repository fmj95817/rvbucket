interface ifetch_if_t #(
    parameter AW = 32,
    parameter DW = 32
);
    logic req_vld;
    logic req_rdy;
    logic [AW-1:0] req_pc;

    logic rsp_vld;
    logic rsp_rdy;
    logic [DW-1:0] rsp_ir;

    modport master (
        output req_vld, req_pc, input req_rdy,
        input rsp_vld, rsp_ir, output rsp_rdy
    );

    modport slave (
        input req_vld, req_pc, output req_rdy,
        output rsp_vld, rsp_ir, input rsp_rdy
    );
endinterface

interface iexec_if_t #(
    parameter AW = 32,
    parameter DW = 32
);
    logic req_vld;
    logic req_rdy;
    struct packed {
        logic [DW-1:0] ir;
        logic [AW-1:0] pc;
    } req_pkt;

    struct packed {
        logic taken;
        logic [DW-1:0] offset;
    } rsp_pkt;

    modport master (
        output req_vld, req_pkt, input req_rdy,
        input rsp_pkt
    );

    modport slave (
        input req_vld, req_pkt, output req_rdy,
        output rsp_pkt
    );
endinterface

interface ldst_if_t #(
    parameter AW = 32,
    parameter DW = 32
);
    logic req_vld;
    logic req_rdy;
    struct packed {
        logic [AW-1:0] addr;
        logic          wr;
        logic [DW-1:0] data;
        logic [3:0]    strobe;
    } req_pkt;

    logic rsp_vld;
    logic rsp_rdy;
    struct packed {
        logic [DW-1:0] data;
        logic          ok;
    } rsp_pkt;

    modport master (
        output req_vld, req_pkt, input req_rdy,
        input rsp_pkt
    );

    modport slave (
        input req_vld, req_pkt, output req_rdy,
        output rsp_pkt
    );
endinterface

interface sram_if_t #(
    parameter AW = 15,
    parameter DW = 32
);
    logic  [AW-1:0] addr;
    logic           wen;
    logic  [DW-1:0] wdata;
    logic  [DW-1:0] rdata;

    modport master (output addr, wen, wdata, input rdata);
    modport slave (input addr, wen, wdata, output rdata);
endinterface
