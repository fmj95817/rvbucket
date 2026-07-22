`include "itf/bti_req_if.svh"
`include "itf/hart_expt_if.svh"
`include "itf/ldst_req_if.svh"
`include "spec/core/hart.svh"

module hbi #(
    parameter int STG_FIFO_DEPTH = `HART_HBI_STG_FIFO_DEPTH,
    parameter int I_OST_DEPTH = `HART_HBI_I_OST_DEPTH,
    parameter int D_OST_DEPTH = `HART_HBI_D_OST_DEPTH
)(
    input logic       clk,
    input logic       rst_n,
    fch_req_if_t.slv  fch_req_slv,
    fch_rsp_if_t.mst  fch_rsp_mst,
    hart_expt_if_t.slv mmu_fch_expt_slv,
    ldst_req_if_t.slv ldst_req_slv,
    ldst_rsp_if_t.mst ldst_rsp_mst,
    bti_req_if_t.mst  i_bti_req_mst,
    bti_rsp_if_t.slv  i_bti_rsp_slv,
    bti_req_if_t.mst  d_bti_req_mst,
    bti_rsp_if_t.slv  d_bti_rsp_slv
);
    typedef struct packed {
        logic [31:0] pc;
    } fch_req_pkt_t;

    typedef struct packed {
        hart_expt_type_t expt_type;
        hart_expt_cause_t cause;
        logic [1:0] priv;
        logic [31:0] pc;
        logic [31:0] tval;
    } hart_expt_pkt_t;

    typedef struct packed {
        logic [31:0] ir;
        logic ok;
        logic expt;
        logic [31:0] cause;
        logic [1:0] priv;
        logic [31:0] tval;
    } fch_rsp_pkt_t;

    typedef struct packed {
        logic [31:0] addr;
        logic st;
        ldst_req_cmo_t cmo;
        ldst_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } ldst_req_pkt_t;

    typedef struct packed {
        logic [31:0] data;
        logic ok;
    } ldst_rsp_pkt_t;

    typedef struct packed {
        logic [31:0] pc;
        logic rsp_vld;
        fch_rsp_pkt_t rsp;
    } i_ost_ctx_t;

    typedef struct packed {
        logic rsp_vld;
        ldst_rsp_pkt_t rsp;
    } d_ost_ctx_t;

    localparam int FCH_REQ_DW = $bits(fch_req_pkt_t);
    localparam int LDST_REQ_DW = $bits(ldst_req_pkt_t);
    localparam int I_OST_CTX_DW = $bits(i_ost_ctx_t);
    localparam int D_OST_CTX_DW = $bits(d_ost_ctx_t);
    localparam int I_OST_SLOT_W = $clog2(I_OST_DEPTH);
    localparam int D_OST_SLOT_W = $clog2(D_OST_DEPTH);

    logic fch_fifo_rd_vld;
    logic fch_fifo_rd_rdy;
    logic [FCH_REQ_DW-1:0] fch_fifo_rd_data;
    logic [FCH_REQ_DW-1:0] fch_fifo_wr_data;
    fch_req_pkt_t fch_req_pkt;

    logic ldst_fifo_rd_vld;
    logic ldst_fifo_rd_rdy;
    logic [LDST_REQ_DW-1:0] ldst_fifo_rd_data;
    logic [LDST_REQ_DW-1:0] ldst_fifo_wr_data;
    ldst_req_pkt_t ldst_req_pkt;

    logic i_ost_alloc_rdy;
    logic [I_OST_SLOT_W-1:0] i_ost_alloc_slot;
    logic i_ost_head_vld;
    logic [I_OST_CTX_DW-1:0] i_ost_head_ctx_raw;
    logic [I_OST_SLOT_W-1:0] i_ost_head_slot;
    i_ost_ctx_t i_ost_head_ctx;
    logic i_ost_free_head;
    logic i_ost_slot_wr_vld;
    logic [I_OST_SLOT_W-1:0] i_ost_slot_wr_idx;
    logic [I_OST_CTX_DW-1:0] i_ost_slot_wr_ctx_raw;
    logic [I_OST_DEPTH-1:0] i_ost_slot_vld;
    logic [I_OST_DEPTH*I_OST_CTX_DW-1:0] i_ost_slot_ctx_flat;
    i_ost_ctx_t i_ost_slot_wr_ctx;
    logic i_wait_found;
    logic [I_OST_SLOT_W-1:0] i_wait_slot;
    i_ost_ctx_t i_wait_ctx;
    logic i_expt_q_vld;
    logic i_expt_take;
    hart_expt_pkt_t i_expt_q_pkt;
    hart_expt_pkt_t i_expt_pkt;
    logic i_expt_found;
    logic [I_OST_SLOT_W-1:0] i_expt_slot;
    i_ost_ctx_t i_expt_ctx;

    logic d_ost_alloc_rdy;
    logic [D_OST_SLOT_W-1:0] d_ost_alloc_slot;
    logic d_ost_head_vld;
    logic [D_OST_CTX_DW-1:0] d_ost_head_ctx_raw;
    logic [D_OST_SLOT_W-1:0] d_ost_head_slot;
    d_ost_ctx_t d_ost_head_ctx;
    logic d_ost_free_head;
    logic d_ost_slot_wr_vld;
    logic [D_OST_SLOT_W-1:0] d_ost_slot_wr_idx;
    logic [D_OST_CTX_DW-1:0] d_ost_slot_wr_ctx_raw;
    logic [D_OST_DEPTH-1:0] d_ost_slot_vld;
    logic [D_OST_DEPTH*D_OST_CTX_DW-1:0] d_ost_slot_ctx_flat;
    d_ost_ctx_t d_ost_slot_wr_ctx;
    logic d_wait_found;
    logic [D_OST_SLOT_W-1:0] d_wait_slot;
    d_ost_ctx_t d_wait_ctx;

    wire i_req_hsk = i_bti_req_mst.vld && i_bti_req_mst.rdy;
    wire d_req_hsk = d_bti_req_mst.vld && d_bti_req_mst.rdy;
    wire i_rsp_hsk = i_bti_rsp_slv.vld && i_bti_rsp_slv.rdy;
    wire d_rsp_hsk = d_bti_rsp_slv.vld && d_bti_rsp_slv.rdy;
    wire i_expt_vld = i_expt_q_vld;
    wire fch_rsp_hsk = fch_rsp_mst.vld && fch_rsp_mst.rdy;
    wire ldst_rsp_hsk = ldst_rsp_mst.vld && ldst_rsp_mst.rdy;

    assign fch_fifo_wr_data = fch_req_slv.pkt;
    assign ldst_fifo_wr_data = ldst_req_slv.pkt;
    assign fch_req_pkt = fch_fifo_rd_data;
    assign ldst_req_pkt = ldst_fifo_rd_data;
    assign i_expt_pkt = i_expt_q_pkt;
    assign i_ost_head_ctx = i_ost_head_ctx_raw;
    assign d_ost_head_ctx = d_ost_head_ctx_raw;
    assign i_ost_slot_wr_ctx_raw = i_ost_slot_wr_ctx;
    assign d_ost_slot_wr_ctx_raw = d_ost_slot_wr_ctx;

    function automatic i_ost_ctx_t i_ctx_at(
        input logic [I_OST_DEPTH*I_OST_CTX_DW-1:0] flat,
        input logic [I_OST_SLOT_W-1:0] idx
    );
        i_ctx_at = flat[idx * I_OST_CTX_DW +: I_OST_CTX_DW];
    endfunction

    function automatic d_ost_ctx_t d_ctx_at(
        input logic [D_OST_DEPTH*D_OST_CTX_DW-1:0] flat,
        input logic [D_OST_SLOT_W-1:0] idx
    );
        d_ctx_at = flat[idx * D_OST_CTX_DW +: D_OST_CTX_DW];
    endfunction

    fifo #(
        .DW           (FCH_REQ_DW),
        .DEPTH        (STG_FIFO_DEPTH),
        .FALL_THROUGH (1'b1)
    ) u_fch_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (fch_req_slv.vld),
        .wr_rdy  (fch_req_slv.rdy),
        .wr_data (fch_fifo_wr_data),
        .rd_vld  (fch_fifo_rd_vld),
        .rd_rdy  (fch_fifo_rd_rdy),
        .rd_data (fch_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW           (LDST_REQ_DW),
        .DEPTH        (STG_FIFO_DEPTH),
        .FALL_THROUGH (1'b1)
    ) u_ldst_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (ldst_req_slv.vld),
        .wr_rdy  (ldst_req_slv.rdy),
        .wr_data (ldst_fifo_wr_data),
        .rd_vld  (ldst_fifo_rd_vld),
        .rd_rdy  (ldst_fifo_rd_rdy),
        .rd_data (ldst_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    ostq #(
        .DW    (I_OST_CTX_DW),
        .DEPTH (I_OST_DEPTH)
    ) u_i_ost(
        .clk           (clk),
        .rst_n         (rst_n),
        .alloc_vld     (i_req_hsk),
        .alloc_rdy     (i_ost_alloc_rdy),
        .alloc_ctx     (i_ost_ctx_t'{pc:fch_req_pkt.pc, rsp_vld:1'b0,
            rsp:'0}),
        .alloc_slot    (i_ost_alloc_slot),
        .head_vld      (i_ost_head_vld),
        .head_ctx      (i_ost_head_ctx_raw),
        .head_slot     (i_ost_head_slot),
        .free_head     (i_ost_free_head),
        .slot_wr_vld   (i_ost_slot_wr_vld),
        .slot_wr_idx   (i_ost_slot_wr_idx),
        .slot_wr_ctx   (i_ost_slot_wr_ctx_raw),
        .slot_vld      (i_ost_slot_vld),
        .slot_ctx_flat (i_ost_slot_ctx_flat),
        .empty         (),
        .full          ()
    );

    ostq #(
        .DW    (D_OST_CTX_DW),
        .DEPTH (D_OST_DEPTH)
    ) u_d_ost(
        .clk           (clk),
        .rst_n         (rst_n),
        .alloc_vld     (d_req_hsk),
        .alloc_rdy     (d_ost_alloc_rdy),
        .alloc_ctx     ('0),
        .alloc_slot    (d_ost_alloc_slot),
        .head_vld      (d_ost_head_vld),
        .head_ctx      (d_ost_head_ctx_raw),
        .head_slot     (d_ost_head_slot),
        .free_head     (d_ost_free_head),
        .slot_wr_vld   (d_ost_slot_wr_vld),
        .slot_wr_idx   (d_ost_slot_wr_idx),
        .slot_wr_ctx   (d_ost_slot_wr_ctx_raw),
        .slot_vld      (d_ost_slot_vld),
        .slot_ctx_flat (d_ost_slot_ctx_flat),
        .empty         (),
        .full          ()
    );

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            i_expt_q_vld <= 1'b0;
            i_expt_q_pkt <= '0;
        end else if (!i_expt_q_vld || i_expt_take) begin
            i_expt_q_vld <= mmu_fch_expt_slv.vld;
            if (mmu_fch_expt_slv.vld) begin
                i_expt_q_pkt <= mmu_fch_expt_slv.pkt;
            end
        end
    end

    always_comb begin
        i_wait_found = 1'b0;
        i_wait_slot = '0;
        i_wait_ctx = '0;
        for (int unsigned i = 0; i < I_OST_DEPTH; i++) begin
            logic [I_OST_SLOT_W-1:0] idx;
            i_ost_ctx_t ctx;
            idx = i_ost_head_slot + I_OST_SLOT_W'(i);
            ctx = i_ctx_at(i_ost_slot_ctx_flat, idx);
            if (!i_wait_found && i_ost_slot_vld[idx] && !ctx.rsp_vld) begin
                i_wait_found = 1'b1;
                i_wait_slot = idx;
                i_wait_ctx = ctx;
            end
        end
    end

    always_comb begin
        i_expt_found = 1'b0;
        i_expt_slot = '0;
        i_expt_ctx = '0;
        for (int unsigned i = 0; i < I_OST_DEPTH; i++) begin
            logic [I_OST_SLOT_W-1:0] idx;
            i_ost_ctx_t ctx;
            idx = i_ost_head_slot + I_OST_SLOT_W'(i);
            ctx = i_ctx_at(i_ost_slot_ctx_flat, idx);
            if (!i_expt_found && i_ost_slot_vld[idx] && !ctx.rsp_vld &&
                ctx.pc == i_expt_pkt.pc) begin
                i_expt_found = 1'b1;
                i_expt_slot = idx;
                i_expt_ctx = ctx;
            end
        end
    end

    always_comb begin
        d_wait_found = 1'b0;
        d_wait_slot = '0;
        d_wait_ctx = '0;
        for (int unsigned i = 0; i < D_OST_DEPTH; i++) begin
            logic [D_OST_SLOT_W-1:0] idx;
            d_ost_ctx_t ctx;
            idx = d_ost_head_slot + D_OST_SLOT_W'(i);
            ctx = d_ctx_at(d_ost_slot_ctx_flat, idx);
            if (!d_wait_found && d_ost_slot_vld[idx] && !ctx.rsp_vld) begin
                d_wait_found = 1'b1;
                d_wait_slot = idx;
                d_wait_ctx = ctx;
            end
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && i_bti_rsp_slv.vld) begin
            assert (i_bti_rsp_slv.pkt.trans_id == `FCH_TRANS_ID)
                else $fatal(1, "HBI I response trans_id mismatch");
            assert (i_wait_found)
                else $fatal(1, "HBI I response without outstanding entry");
        end
        if (rst_n && i_expt_vld) begin
            assert (i_expt_found)
                else $fatal(1, "HBI I exception without outstanding entry");
        end
        if (rst_n && mmu_fch_expt_slv.vld && i_expt_q_vld && !i_expt_take) begin
            $fatal(1, "HBI I exception stage overflow");
        end
        if (rst_n && d_bti_rsp_slv.vld) begin
            assert (d_bti_rsp_slv.pkt.trans_id == `LDST_TRANS_ID)
                else $fatal(1, "HBI D response trans_id mismatch");
            assert (d_wait_found)
                else $fatal(1, "HBI D response without outstanding entry");
        end
    end
`endif

    assign i_bti_req_mst.vld = fch_fifo_rd_vld && i_ost_alloc_rdy;
    assign i_bti_req_mst.pkt.trans_id = `FCH_TRANS_ID;
    assign i_bti_req_mst.pkt.cmd = BTI_REQ_CMD_READ;
    assign i_bti_req_mst.pkt.addr = fch_req_pkt.pc;
    assign i_bti_req_mst.pkt.size = BTI_REQ_SIZE_B4;
    assign i_bti_req_mst.pkt.data = '0;
    assign i_bti_req_mst.pkt.strobe = '0;
    assign fch_fifo_rd_rdy = i_req_hsk;

    assign d_bti_req_mst.vld = ldst_fifo_rd_vld && d_ost_alloc_rdy;
    assign d_bti_req_mst.pkt.trans_id = `LDST_TRANS_ID;
    always_comb begin
        unique case (ldst_req_pkt.cmo)
        LDST_REQ_CMO_INVAL:
            d_bti_req_mst.pkt.cmd = BTI_REQ_CMD_CBO_INVAL;
        LDST_REQ_CMO_CLEAN:
            d_bti_req_mst.pkt.cmd = BTI_REQ_CMD_CBO_CLEAN;
        LDST_REQ_CMO_FLUSH:
            d_bti_req_mst.pkt.cmd = BTI_REQ_CMD_CBO_FLUSH;
        default:
            d_bti_req_mst.pkt.cmd = ldst_req_pkt.st ?
                BTI_REQ_CMD_WRITE : BTI_REQ_CMD_READ;
        endcase
    end
    assign d_bti_req_mst.pkt.addr = ldst_req_pkt.addr;
    assign d_bti_req_mst.pkt.size = bti_req_size_t'(ldst_req_pkt.size);
    assign d_bti_req_mst.pkt.data = ldst_req_pkt.data;
    assign d_bti_req_mst.pkt.strobe = ldst_req_pkt.strobe;
    assign ldst_fifo_rd_rdy = d_req_hsk;

    assign i_bti_rsp_slv.rdy = i_wait_found && !i_expt_vld;
    assign i_ost_slot_wr_vld = i_expt_vld || i_rsp_hsk;
    assign i_ost_slot_wr_idx = i_expt_vld ? i_expt_slot : i_wait_slot;
    assign i_expt_take = i_expt_vld && i_expt_found;
    always_comb begin
        if (i_expt_vld) begin
            i_ost_slot_wr_ctx = i_expt_ctx;
            i_ost_slot_wr_ctx.rsp_vld = 1'b1;
            i_ost_slot_wr_ctx.rsp.ir = '0;
            i_ost_slot_wr_ctx.rsp.ok = 1'b0;
            i_ost_slot_wr_ctx.rsp.expt = 1'b1;
            i_ost_slot_wr_ctx.rsp.cause = {27'b0, i_expt_pkt.cause};
            i_ost_slot_wr_ctx.rsp.priv = i_expt_pkt.priv;
            i_ost_slot_wr_ctx.rsp.tval = i_expt_pkt.tval;
        end else begin
            i_ost_slot_wr_ctx = i_wait_ctx;
            i_ost_slot_wr_ctx.rsp_vld = 1'b1;
            i_ost_slot_wr_ctx.rsp.ir = i_bti_rsp_slv.pkt.data;
            i_ost_slot_wr_ctx.rsp.ok = i_bti_rsp_slv.pkt.ok;
            i_ost_slot_wr_ctx.rsp.expt = 1'b0;
            i_ost_slot_wr_ctx.rsp.cause = '0;
            i_ost_slot_wr_ctx.rsp.priv = '0;
            i_ost_slot_wr_ctx.rsp.tval = '0;
        end
    end

    assign d_bti_rsp_slv.rdy = d_wait_found;
    assign d_ost_slot_wr_vld = d_rsp_hsk;
    assign d_ost_slot_wr_idx = d_wait_slot;
    always_comb begin
        d_ost_slot_wr_ctx = d_wait_ctx;
        d_ost_slot_wr_ctx.rsp_vld = 1'b1;
        d_ost_slot_wr_ctx.rsp.data = d_bti_rsp_slv.pkt.data;
        d_ost_slot_wr_ctx.rsp.ok = d_bti_rsp_slv.pkt.ok;
    end

    assign fch_rsp_mst.vld = i_ost_head_vld && i_ost_head_ctx.rsp_vld;
    assign fch_rsp_mst.pkt = i_ost_head_ctx.rsp;
    assign i_ost_free_head = fch_rsp_hsk;

    assign ldst_rsp_mst.vld = d_ost_head_vld && d_ost_head_ctx.rsp_vld;
    assign ldst_rsp_mst.pkt = d_ost_head_ctx.rsp;
    assign d_ost_free_head = ldst_rsp_hsk;

    wire unused_i_ost = |i_ost_alloc_slot;
    wire unused_d_ost = |d_ost_alloc_slot;
endmodule
