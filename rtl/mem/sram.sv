`include "itf/bti_req_if.svh"

interface sram_rw_if_t #(
    parameter AW = 15,
    parameter DW = 32
);
    logic           cs;
    logic  [AW-1:0] addr;
    logic           wen;
    logic  [DW-1:0] wdata;
    logic  [DW-1:0] rdata;

    modport mst (output cs, addr, wen, wdata, input rdata);
    modport slv (input cs, addr, wen, wdata, output rdata);
endinterface

module bti_to_sram #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter SRAM_AW = 15
)(
    input logic        clk,
    input logic        rst_n,
    bti_req_if_t.slv   bti_req_slv,
    bti_rsp_if_t.mst   bti_rsp_mst,
    sram_rw_if_t.mst   sram_bank0_rw_mst,
    sram_rw_if_t.mst   sram_bank1_rw_mst,
    sram_rw_if_t.mst   sram_bank2_rw_mst,
    sram_rw_if_t.mst   sram_bank3_rw_mst
);
    localparam SRAM_WORD_AW = SRAM_AW - 2;

    reg_slice #(
        .DW(`BTI_TIDW)
    ) u_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .src_vld  (bti_req_slv.vld),
        .src_rdy  (bti_req_slv.rdy),
        .src_data (bti_req_slv.pkt.tid),
        .dst_vld  (bti_rsp_mst.vld),
        .dst_rdy  (bti_rsp_mst.rdy),
        .dst_data (bti_rsp_mst.pkt.tid)
    );

    tri cs = bti_req_slv.vld & bti_req_slv.rdy;
    tri wen = bti_req_slv.pkt.cmd == BTI_CMD_WRITE;
    tri [3:0] strobe = bti_req_slv.pkt.strobe;

    assign sram_bank0_rw_mst.cs = cs;
    assign sram_bank1_rw_mst.cs = cs;
    assign sram_bank2_rw_mst.cs = cs;
    assign sram_bank3_rw_mst.cs = cs;

    tri [BTI_AW-1:0] addr = bti_req_slv.pkt.addr;
    tri [SRAM_WORD_AW-1:0] sram_addr = addr[SRAM_WORD_AW+1:2];
    tri [SRAM_WORD_AW-1:0] sram_addr_nxt = sram_addr + 1'b1;
    tri [BTI_DW-1:0] wdata = bti_req_slv.pkt.data;

    always_comb begin
        case (addr[1:0])
            2'b00: begin
                sram_bank0_rw_mst.addr = sram_addr;
                sram_bank1_rw_mst.addr = sram_addr;
                sram_bank2_rw_mst.addr = sram_addr;
                sram_bank3_rw_mst.addr = sram_addr;
                sram_bank0_rw_mst.wen = wen & strobe[0];
                sram_bank1_rw_mst.wen = wen & strobe[1];
                sram_bank2_rw_mst.wen = wen & strobe[2];
                sram_bank3_rw_mst.wen = wen & strobe[3];
                sram_bank0_rw_mst.wdata = wdata[7:0];
                sram_bank1_rw_mst.wdata = wdata[15:8];
                sram_bank2_rw_mst.wdata = wdata[23:16];
                sram_bank3_rw_mst.wdata = wdata[31:24];
            end
            2'b01: begin
                sram_bank0_rw_mst.addr = sram_addr_nxt;
                sram_bank1_rw_mst.addr = sram_addr;
                sram_bank2_rw_mst.addr = sram_addr;
                sram_bank3_rw_mst.addr = sram_addr;
                sram_bank0_rw_mst.wen = wen & strobe[3];
                sram_bank1_rw_mst.wen = wen & strobe[0];
                sram_bank2_rw_mst.wen = wen & strobe[1];
                sram_bank3_rw_mst.wen = wen & strobe[2];
                sram_bank0_rw_mst.wdata = wdata[31:24];
                sram_bank1_rw_mst.wdata = wdata[7:0];
                sram_bank2_rw_mst.wdata = wdata[15:8];
                sram_bank3_rw_mst.wdata = wdata[23:16];
            end
            2'b10: begin
                sram_bank0_rw_mst.addr = sram_addr_nxt;
                sram_bank1_rw_mst.addr = sram_addr_nxt;
                sram_bank2_rw_mst.addr = sram_addr;
                sram_bank3_rw_mst.addr = sram_addr;
                sram_bank0_rw_mst.wen = wen & strobe[2];
                sram_bank1_rw_mst.wen = wen & strobe[3];
                sram_bank2_rw_mst.wen = wen & strobe[0];
                sram_bank3_rw_mst.wen = wen & strobe[1];
                sram_bank0_rw_mst.wdata = wdata[23:16];
                sram_bank1_rw_mst.wdata = wdata[31:24];
                sram_bank2_rw_mst.wdata = wdata[7:0];
                sram_bank3_rw_mst.wdata = wdata[15:8];
            end
            2'b11: begin
                sram_bank0_rw_mst.addr = sram_addr_nxt;
                sram_bank1_rw_mst.addr = sram_addr_nxt;
                sram_bank2_rw_mst.addr = sram_addr_nxt;
                sram_bank3_rw_mst.addr = sram_addr;
                sram_bank0_rw_mst.wen = wen & strobe[1];
                sram_bank1_rw_mst.wen = wen & strobe[2];
                sram_bank2_rw_mst.wen = wen & strobe[3];
                sram_bank3_rw_mst.wen = wen & strobe[0];
                sram_bank0_rw_mst.wdata = wdata[15:8];
                sram_bank1_rw_mst.wdata = wdata[23:16];
                sram_bank2_rw_mst.wdata = wdata[31:24];
                sram_bank3_rw_mst.wdata = wdata[7:0];
            end
            default: begin
                sram_bank0_rw_mst.addr = {SRAM_WORD_AW{1'bx}};
                sram_bank1_rw_mst.addr = {SRAM_WORD_AW{1'bx}};
                sram_bank2_rw_mst.addr = {SRAM_WORD_AW{1'bx}};
                sram_bank3_rw_mst.addr = {SRAM_WORD_AW{1'bx}};
                sram_bank0_rw_mst.wen = 1'bx;
                sram_bank1_rw_mst.wen = 1'bx;
                sram_bank2_rw_mst.wen = 1'bx;
                sram_bank3_rw_mst.wen = 1'bx;
                sram_bank0_rw_mst.wdata = {8{1'bx}};
                sram_bank1_rw_mst.wdata = {8{1'bx}};
                sram_bank2_rw_mst.wdata = {8{1'bx}};
                sram_bank3_rw_mst.wdata = {8{1'bx}};
            end
        endcase
    end

    always_comb begin
        case (addr[1:0])
            2'b00: bti_rsp_mst.pkt.data = {
                sram_bank3_rw_mst.rdata,
                sram_bank2_rw_mst.rdata,
                sram_bank1_rw_mst.rdata,
                sram_bank0_rw_mst.rdata
            };
            2'b01: bti_rsp_mst.pkt.data = {
                sram_bank0_rw_mst.rdata,
                sram_bank3_rw_mst.rdata,
                sram_bank2_rw_mst.rdata,
                sram_bank1_rw_mst.rdata
            };
            2'b10: bti_rsp_mst.pkt.data = {
                sram_bank1_rw_mst.rdata,
                sram_bank0_rw_mst.rdata,
                sram_bank3_rw_mst.rdata,
                sram_bank2_rw_mst.rdata
            };
            2'b11: bti_rsp_mst.pkt.data = {
                sram_bank2_rw_mst.rdata,
                sram_bank1_rw_mst.rdata,
                sram_bank0_rw_mst.rdata,
                sram_bank3_rw_mst.rdata
            };
            default: bti_rsp_mst.pkt.data = {BTI_DW{1'bx}};
        endcase
    end

    assign bti_rsp_mst.pkt.ok = 1'b1;

endmodule

module bti_sram #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter SRAM_AW = 15
)(
    input logic        clk,
    input logic        rst_n,
    bti_req_if_t.slv   bti_req_slv,
    bti_rsp_if_t.mst   bti_rsp_mst
);
    localparam SRAM_WORD_AW = SRAM_AW - 2;

    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank0_rw_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank1_rw_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank2_rw_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank3_rw_if();

    sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank0(
        .clk         (clk),
        .sram_rw_slv (sram_bank0_rw_if)
    );

    sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank1(
        .clk         (clk),
        .sram_rw_slv (sram_bank1_rw_if)
    );

    sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank2(
        .clk         (clk),
        .sram_rw_slv (sram_bank2_rw_if)
    );

    sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank3(
        .clk         (clk),
        .sram_rw_slv (sram_bank3_rw_if)
    );

    bti_to_sram #(
        .BTI_AW            (BTI_AW),
        .BTI_DW            (BTI_DW),
        .SRAM_AW           (SRAM_AW)
    ) u_bti_to_sram(
        .clk               (clk),
        .rst_n             (rst_n),
        .bti_req_slv       (bti_req_slv),
        .bti_rsp_mst       (bti_rsp_mst),
        .sram_bank0_rw_mst (sram_bank0_rw_if),
        .sram_bank1_rw_mst (sram_bank1_rw_if),
        .sram_bank2_rw_mst (sram_bank2_rw_if),
        .sram_bank3_rw_mst (sram_bank3_rw_if)
    );

endmodule

module bti_dp_sram #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter SRAM_AW = 15
)(
    input logic        clk,
    input logic        rst_n,
    bti_req_if_t.slv   bti_r_req_slv,
    bti_rsp_if_t.mst   bti_r_rsp_mst,
    bti_req_if_t.slv   bti_w_req_slv,
    bti_rsp_if_t.mst   bti_w_rsp_mst
);
    localparam SRAM_WORD_AW = SRAM_AW - 2;

    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank0_r_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank0_w_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank1_r_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank1_w_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank2_r_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank2_w_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank3_r_if();
    sram_rw_if_t #(SRAM_WORD_AW, BTI_DW/4) sram_bank3_w_if();

    dp_sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank0(
        .clk         (clk),
        .sram_r_slv  (sram_bank0_r_if),
        .sram_w_slv  (sram_bank0_w_if)
    );

    dp_sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank1(
        .clk         (clk),
        .sram_r_slv  (sram_bank1_r_if),
        .sram_w_slv  (sram_bank1_w_if)
    );

    dp_sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank2(
        .clk         (clk),
        .sram_r_slv  (sram_bank2_r_if),
        .sram_w_slv  (sram_bank2_w_if)
    );

    dp_sram #(
        .AW          (SRAM_WORD_AW),
        .DW          (BTI_DW/4)
    ) u_sram_bank3(
        .clk         (clk),
        .sram_r_slv  (sram_bank3_r_if),
        .sram_w_slv  (sram_bank3_w_if)
    );

    bti_to_sram #(
        .BTI_AW            (BTI_AW),
        .BTI_DW            (BTI_DW),
        .SRAM_AW           (SRAM_AW)
    ) u_bti_r_to_sram(
        .clk               (clk),
        .rst_n             (rst_n),
        .bti_req_slv       (bti_r_req_slv),
        .bti_rsp_mst       (bti_r_rsp_mst),
        .sram_bank0_rw_mst (sram_bank0_r_if),
        .sram_bank1_rw_mst (sram_bank1_r_if),
        .sram_bank2_rw_mst (sram_bank2_r_if),
        .sram_bank3_rw_mst (sram_bank3_r_if)
    );

    bti_to_sram #(
        .BTI_AW            (BTI_AW),
        .BTI_DW            (BTI_DW),
        .SRAM_AW           (SRAM_AW)
    ) u_bti_w_to_sram(
        .clk               (clk),
        .rst_n             (rst_n),
        .bti_req_slv       (bti_w_req_slv),
        .bti_rsp_mst       (bti_w_rsp_mst),
        .sram_bank0_rw_mst (sram_bank0_w_if),
        .sram_bank1_rw_mst (sram_bank1_w_if),
        .sram_bank2_rw_mst (sram_bank2_w_if),
        .sram_bank3_rw_mst (sram_bank3_w_if)
    );

endmodule