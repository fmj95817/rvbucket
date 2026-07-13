`include "itf/bti_req_if.svh"
`include "itf/hart_expt_if.svh"
`include "spec/core/hart.svh"

module mmu #(
    parameter int I_STG_FIFO_DEPTH = `HART_MMU_I_STG_FIFO_DEPTH,
    parameter int D_STG_FIFO_DEPTH = `HART_MMU_D_STG_FIFO_DEPTH,
    parameter int OST_DEPTH = `HART_MMU_OST_DEPTH
)(
    input logic clk,
    input logic rst_n,
    bti_req_if_t.slv va_i_req_slv,
    bti_rsp_if_t.mst va_i_rsp_mst,
    bti_req_if_t.slv va_d_req_slv,
    bti_rsp_if_t.mst va_d_rsp_mst,
    bti_req_if_t.mst pa_i_req_mst,
    bti_rsp_if_t.slv pa_i_rsp_slv,
    bti_req_if_t.mst pa_d_req_mst,
    bti_rsp_if_t.slv pa_d_rsp_slv,
    exu_state_if_t.slv exu_state_slv,
    csr_mmu_state_if_t.slv csr_mmu_state_slv,
    tlb_flush_if_t.slv tlb_flush_slv,
    hart_expt_if_t.mst fch_expt_mst,
    hart_expt_if_t.mst ldst_expt_mst
);
    localparam logic [15:0] MMU_PTE_TRANS_ID = 16'hfffe;
    localparam int I_OST_SLOT_W = $clog2(OST_DEPTH);
    localparam int D_OST_SLOT_W = $clog2(OST_DEPTH);

    typedef struct packed {
        logic [15:0] trans_id;
        bti_req_cmd_t cmd;
        logic [31:0] addr;
        bti_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } bti_req_pkt_t;

    typedef struct packed {
        logic [15:0] trans_id;
        logic [31:0] data;
        logic ok;
    } bti_rsp_pkt_t;

    typedef struct packed {
        hart_expt_type_t expt_type;
        hart_expt_cause_t cause;
        logic [1:0] priv;
        logic [31:0] pc;
        logic [31:0] tval;
    } hart_expt_pkt_t;

    typedef struct packed {
        logic is_inst;
        bti_req_pkt_t req;
        logic pa_issued;
        logic ready;
        logic expt_vld;
        hart_expt_pkt_t expt;
        bti_rsp_pkt_t rsp;
    } ost_ctx_t;

    typedef enum logic [2:0] {
        WALK_IDLE,
        WALK_WAIT_OST_DRAIN,
        WALK_SEND_PTE,
        WALK_WAIT_PTE,
        WALK_SEND_FINAL,
        WALK_WAIT_FINAL
    } walk_state_t;

    localparam int BTI_REQ_DW = $bits(bti_req_pkt_t);
    localparam int OST_CTX_DW = $bits(ost_ctx_t);

    logic i_fifo_rd_vld;
    logic i_fifo_rd_rdy;
    logic i_fifo_wr_rdy;
    logic [BTI_REQ_DW-1:0] i_fifo_rd_data;
    bti_req_pkt_t i_req_pkt;
    logic d_fifo_rd_vld;
    logic d_fifo_rd_rdy;
    logic d_fifo_wr_rdy;
    logic [BTI_REQ_DW-1:0] d_fifo_rd_data;
    bti_req_pkt_t d_req_pkt;

    logic i_ost_alloc_vld;
    logic i_ost_alloc_rdy;
    logic [I_OST_SLOT_W-1:0] i_ost_alloc_slot;
    ost_ctx_t i_ost_alloc_ctx;
    logic i_ost_head_vld;
    ost_ctx_t i_ost_head_ctx;
    logic [I_OST_SLOT_W-1:0] i_ost_head_slot;
    logic i_ost_free_head;
    logic i_ost_slot_wr_vld;
    logic [I_OST_SLOT_W-1:0] i_ost_slot_wr_idx;
    ost_ctx_t i_ost_slot_wr_ctx;
    logic [OST_DEPTH-1:0] i_ost_slot_vld;
    logic [OST_DEPTH*OST_CTX_DW-1:0] i_ost_slot_ctx_flat;
    ost_ctx_t i_slot_ctx[OST_DEPTH];

    logic d_ost_alloc_vld;
    logic d_ost_alloc_rdy;
    logic [D_OST_SLOT_W-1:0] d_ost_alloc_slot;
    ost_ctx_t d_ost_alloc_ctx;
    logic d_ost_head_vld;
    ost_ctx_t d_ost_head_ctx;
    logic [D_OST_SLOT_W-1:0] d_ost_head_slot;
    logic d_ost_free_head;
    logic d_ost_slot_wr_vld;
    logic [D_OST_SLOT_W-1:0] d_ost_slot_wr_idx;
    ost_ctx_t d_ost_slot_wr_ctx;
    logic [OST_DEPTH-1:0] d_ost_slot_vld;
    logic [OST_DEPTH*OST_CTX_DW-1:0] d_ost_slot_ctx_flat;
    ost_ctx_t d_slot_ctx[OST_DEPTH];

    logic i_wait_found;
    logic [I_OST_SLOT_W-1:0] i_wait_slot;
    ost_ctx_t i_wait_ctx;
    logic d_wait_found;
    logic [D_OST_SLOT_W-1:0] d_wait_slot;
    ost_ctx_t d_wait_ctx;

    logic lookup_vld_q;
    logic lookup_is_inst_q;
    logic [I_OST_SLOT_W-1:0] lookup_i_slot_q;
    logic [D_OST_SLOT_W-1:0] lookup_d_slot_q;
    bti_req_pkt_t lookup_req_q;
    logic [31:0] lookup_va_q;
    logic [31:0] lookup_pc_q;
    logic [1:0] lookup_priv_q;
    logic lookup_sum_q;
    logic lookup_mxr_q;
    hart_expt_cause_t lookup_fault_cause_q;

    logic pend_pa_vld;
    logic pend_pa_is_inst;
    logic [I_OST_SLOT_W-1:0] pend_pa_i_slot;
    logic [D_OST_SLOT_W-1:0] pend_pa_d_slot;
    bti_req_pkt_t pend_pa_req;

    walk_state_t walk_state;
    logic walk_is_inst;
    logic [I_OST_SLOT_W-1:0] walk_i_slot;
    logic [D_OST_SLOT_W-1:0] walk_d_slot;
    bti_req_pkt_t walk_req;
    logic [31:0] walk_va;
    logic [31:0] walk_pc;
    logic [1:0] walk_priv;
    logic walk_sum;
    logic walk_mxr;
    logic [31:0] walk_root_base;
    logic walk_level;
    logic [31:0] walk_pte;
    logic [21:0] walk_satp_ppn;
    logic [8:0] walk_satp_asid;
    hart_expt_cause_t walk_fault_cause;
    logic fill_tlb_vld;
    logic fill_tlb_is_inst;
    logic [31:0] fill_tlb_va;
    logic [21:0] fill_tlb_satp_ppn;
    logic [8:0] fill_tlb_satp_asid;
    logic fill_tlb_level;
    logic [31:0] fill_tlb_pte;

    logic itlb_lookup_vld;
    logic dtlb_lookup_vld;
    logic itlb_rsp_vld;
    logic itlb_hit;
    logic [31:0] itlb_pte;
    logic itlb_level;
    logic dtlb_rsp_vld;
    logic dtlb_hit;
    logic [31:0] dtlb_pte;
    logic dtlb_level;

    logic new_i_bare_issue;
    logic new_d_bare_issue;
    logic new_i_lookup_issue;
    logic new_d_lookup_issue;
    logic pa_i_from_pend;
    logic pa_d_from_pend;
    logic pa_i_from_walk;
    logic pa_d_from_walk_pte;
    logic pa_d_from_walk_final;
    logic pa_i_from_new_bare;
    logic pa_d_from_new_bare;

    wire tlb_flush = tlb_flush_slv.vld;
    wire [21:0] csr_satp_ppn = csr_mmu_state_slv.pkt.satp[21:0];
    wire [8:0] csr_satp_asid = csr_mmu_state_slv.pkt.satp[30:22];
    wire [1:0] data_priv = exu_state_slv.pkt.priv == 2'b11 &&
        csr_mmu_state_slv.pkt.mstatus[17] ? csr_mmu_state_slv.pkt.mstatus[12:11] :
        exu_state_slv.pkt.priv;
    wire data_sum = csr_mmu_state_slv.pkt.mstatus[18];
    wire data_mxr = csr_mmu_state_slv.pkt.mstatus[19];
    wire i_translate = exu_state_slv.pkt.priv != 2'b11 &&
        csr_mmu_state_slv.pkt.satp[31];
    wire d_translate = data_priv != 2'b11 && csr_mmu_state_slv.pkt.satp[31];
    wire pa_i_req_hsk = pa_i_req_mst.vld && pa_i_req_mst.rdy;
    wire pa_d_req_hsk = pa_d_req_mst.vld && pa_d_req_mst.rdy;
    wire pa_i_rsp_hsk = pa_i_rsp_slv.vld && pa_i_rsp_slv.rdy;
    wire pa_d_rsp_hsk = pa_d_rsp_slv.vld && pa_d_rsp_slv.rdy;
    wire va_i_rsp_hsk = va_i_rsp_mst.vld && va_i_rsp_mst.rdy;
    wire va_d_rsp_hsk = va_d_rsp_mst.vld && va_d_rsp_mst.rdy;
    wire osts_empty = !(|i_ost_slot_vld) && !(|d_ost_slot_vld);
    wire walk_busy = walk_state != WALK_IDLE;

    function automatic logic pte_valid(input logic [31:0] pte);
        pte_valid = pte[0] && !(pte[2] && !pte[1]);
    endfunction

    function automatic logic pte_leaf(input logic [31:0] pte);
        pte_leaf = |pte[3:1];
    endfunction

    function automatic logic pte_permits(
        input logic [31:0] pte,
        input logic is_inst,
        input bti_req_cmd_t cmd,
        input logic [1:0] priv,
        input logic sum,
        input logic mxr
    );
        logic user_ok;
        logic access_ok;
        begin
            user_ok = 1'b1;
            if (priv == 2'b00 && !pte[4])
                user_ok = 1'b0;
            if (priv == 2'b01 && pte[4] && (is_inst || !sum))
                user_ok = 1'b0;
            if (is_inst)
                access_ok = pte[3];
            else if (cmd == BTI_REQ_CMD_WRITE)
                access_ok = pte[2];
            else
                access_ok = pte[1] || (mxr && pte[3]);
            pte_permits = user_ok && access_ok && pte[6] &&
                (is_inst || cmd != BTI_REQ_CMD_WRITE || pte[7]);
        end
    endfunction

    function automatic logic [31:0] leaf_pa(
        input logic [31:0] pte,
        input logic [31:0] va,
        input logic level
    );
        leaf_pa = level ? {pte[29:20], va[21:0]} : {pte[29:10], va[11:0]};
    endfunction

    function automatic logic leaf_pa_ok(
        input logic [31:0] pte,
        input logic level
    );
        leaf_pa_ok = !level || pte[19:10] == 10'b0;
    endfunction

    function automatic logic [31:0] pte_addr(
        input logic [31:0] root_base,
        input logic [31:0] va,
        input logic level
    );
        logic [9:0] vpn;
        begin
            vpn = level ? va[31:22] : va[21:12];
            pte_addr = root_base + {20'b0, vpn, 2'b00};
        end
    endfunction

    function automatic hart_expt_cause_t fault_cause_for(
        input logic is_inst,
        input bti_req_cmd_t cmd
    );
        if (is_inst)
            fault_cause_for = HART_EXPT_CAUSE_INST_PAGE_FAULT;
        else if (cmd == BTI_REQ_CMD_WRITE)
            fault_cause_for = HART_EXPT_CAUSE_STORE_AMO_PAGE_FAULT;
        else
            fault_cause_for = HART_EXPT_CAUSE_LOAD_PAGE_FAULT;
    endfunction

    function automatic ost_ctx_t make_ctx(
        input logic is_inst,
        input bti_req_pkt_t req,
        input logic pa_issued
    );
        begin
            make_ctx = '0;
            make_ctx.is_inst = is_inst;
            make_ctx.req = req;
            make_ctx.pa_issued = pa_issued;
            make_ctx.ready = 1'b0;
            make_ctx.expt_vld = 1'b0;
            make_ctx.rsp.trans_id = req.trans_id;
        end
    endfunction

    function automatic ost_ctx_t complete_rsp(
        input ost_ctx_t ctx,
        input bti_rsp_pkt_t rsp
    );
        begin
            complete_rsp = ctx;
            complete_rsp.rsp = rsp;
            complete_rsp.ready = 1'b1;
            complete_rsp.expt_vld = 1'b0;
        end
    endfunction

    function automatic ost_ctx_t complete_fault(
        input ost_ctx_t ctx,
        input hart_expt_cause_t cause,
        input logic [1:0] priv,
        input logic [31:0] pc,
        input logic [31:0] tval
    );
        begin
            complete_fault = ctx;
            complete_fault.expt_vld = 1'b1;
            complete_fault.expt.expt_type = HART_EXPT_TYPE_EXCEPTION;
            complete_fault.expt.cause = cause;
            complete_fault.expt.priv = priv;
            complete_fault.expt.pc = pc;
            complete_fault.expt.tval = tval;
            complete_fault.rsp.trans_id = ctx.req.trans_id;
            complete_fault.rsp.data = 32'h0;
            complete_fault.rsp.ok = 1'b0;
            complete_fault.ready = 1'b1;
        end
    endfunction

    function automatic ost_ctx_t mark_pa_issued(input ost_ctx_t ctx);
        begin
            mark_pa_issued = ctx;
            mark_pa_issued.pa_issued = 1'b1;
        end
    endfunction

    function automatic logic [I_OST_SLOT_W-1:0] i_slot_idx(input int unsigned idx);
        i_slot_idx = idx[I_OST_SLOT_W-1:0];
    endfunction

    function automatic logic [D_OST_SLOT_W-1:0] d_slot_idx(input int unsigned idx);
        d_slot_idx = idx[D_OST_SLOT_W-1:0];
    endfunction

    fifo #(
        .DW    (BTI_REQ_DW),
        .DEPTH (I_STG_FIFO_DEPTH)
    ) u_i_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (va_i_req_slv.vld && !tlb_flush),
        .wr_rdy  (i_fifo_wr_rdy),
        .wr_data (va_i_req_slv.pkt),
        .rd_vld  (i_fifo_rd_vld),
        .rd_rdy  (i_fifo_rd_rdy),
        .rd_data (i_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW    (BTI_REQ_DW),
        .DEPTH (D_STG_FIFO_DEPTH)
    ) u_d_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (va_d_req_slv.vld && !tlb_flush),
        .wr_rdy  (d_fifo_wr_rdy),
        .wr_data (va_d_req_slv.pkt),
        .rd_vld  (d_fifo_rd_vld),
        .rd_rdy  (d_fifo_rd_rdy),
        .rd_data (d_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    assign i_req_pkt = i_fifo_rd_data;
    assign d_req_pkt = d_fifo_rd_data;
    assign va_i_req_slv.rdy = !tlb_flush && i_fifo_wr_rdy;
    assign va_d_req_slv.rdy = !tlb_flush && d_fifo_wr_rdy;

    ostq #(
        .DW    (OST_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_i_ost(
        .clk           (clk),
        .rst_n         (rst_n),
        .alloc_vld     (i_ost_alloc_vld),
        .alloc_rdy     (i_ost_alloc_rdy),
        .alloc_ctx     (i_ost_alloc_ctx),
        .alloc_slot    (i_ost_alloc_slot),
        .head_vld      (i_ost_head_vld),
        .head_ctx      (i_ost_head_ctx),
        .head_slot     (i_ost_head_slot),
        .free_head     (i_ost_free_head),
        .slot_wr_vld   (i_ost_slot_wr_vld),
        .slot_wr_idx   (i_ost_slot_wr_idx),
        .slot_wr_ctx   (i_ost_slot_wr_ctx),
        .slot_vld      (i_ost_slot_vld),
        .slot_ctx_flat (i_ost_slot_ctx_flat),
        .empty         (),
        .full          ()
    );

    ostq #(
        .DW    (OST_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_d_ost(
        .clk           (clk),
        .rst_n         (rst_n),
        .alloc_vld     (d_ost_alloc_vld),
        .alloc_rdy     (d_ost_alloc_rdy),
        .alloc_ctx     (d_ost_alloc_ctx),
        .alloc_slot    (d_ost_alloc_slot),
        .head_vld      (d_ost_head_vld),
        .head_ctx      (d_ost_head_ctx),
        .head_slot     (d_ost_head_slot),
        .free_head     (d_ost_free_head),
        .slot_wr_vld   (d_ost_slot_wr_vld),
        .slot_wr_idx   (d_ost_slot_wr_idx),
        .slot_wr_ctx   (d_ost_slot_wr_ctx),
        .slot_vld      (d_ost_slot_vld),
        .slot_ctx_flat (d_ost_slot_ctx_flat),
        .empty         (),
        .full          ()
    );

    for (genvar i = 0; i < OST_DEPTH; i++) begin : gen_ost_view
        assign i_slot_ctx[i] = i_ost_slot_ctx_flat[i * OST_CTX_DW +: OST_CTX_DW];
        assign d_slot_ctx[i] = d_ost_slot_ctx_flat[i * OST_CTX_DW +: OST_CTX_DW];
    end

    always_comb begin
        i_wait_found = 1'b0;
        i_wait_slot = '0;
        i_wait_ctx = '0;
        for (int unsigned i = 0; i < OST_DEPTH; i++) begin
            logic [I_OST_SLOT_W-1:0] idx;
            idx = i_ost_head_slot + i_slot_idx(i);
            if (!i_wait_found && i_ost_slot_vld[idx] &&
                i_slot_ctx[idx].pa_issued && !i_slot_ctx[idx].ready) begin
                i_wait_found = 1'b1;
                i_wait_slot = idx;
                i_wait_ctx = i_slot_ctx[idx];
            end
        end
    end

    always_comb begin
        d_wait_found = 1'b0;
        d_wait_slot = '0;
        d_wait_ctx = '0;
        for (int unsigned i = 0; i < OST_DEPTH; i++) begin
            logic [D_OST_SLOT_W-1:0] idx;
            idx = d_ost_head_slot + d_slot_idx(i);
            if (!d_wait_found && d_ost_slot_vld[idx] &&
                d_slot_ctx[idx].pa_issued && !d_slot_ctx[idx].ready) begin
                d_wait_found = 1'b1;
                d_wait_slot = idx;
                d_wait_ctx = d_slot_ctx[idx];
            end
        end
    end

    tlb #(
        .ENTRY_NUM (`MMU_ITLB_SIZE)
    ) u_itlb(
        .clk              (clk),
        .rst_n            (rst_n),
        .flush            (tlb_flush),
        .lookup_vld       (itlb_lookup_vld),
        .lookup_va        (i_req_pkt.addr),
        .lookup_satp_ppn  (csr_satp_ppn),
        .lookup_satp_asid (csr_satp_asid),
        .lookup_rsp_vld   (itlb_rsp_vld),
        .lookup_hit       (itlb_hit),
        .lookup_pte       (itlb_pte),
        .lookup_level     (itlb_level),
        .fill_vld         (fill_tlb_vld && fill_tlb_is_inst),
        .fill_va          (fill_tlb_va),
        .fill_satp_ppn    (fill_tlb_satp_ppn),
        .fill_satp_asid   (fill_tlb_satp_asid),
        .fill_level       (fill_tlb_level),
        .fill_pte         (fill_tlb_pte)
    );

    tlb #(
        .ENTRY_NUM (`MMU_DTLB_SIZE)
    ) u_dtlb(
        .clk              (clk),
        .rst_n            (rst_n),
        .flush            (tlb_flush),
        .lookup_vld       (dtlb_lookup_vld),
        .lookup_va        (d_req_pkt.addr),
        .lookup_satp_ppn  (csr_satp_ppn),
        .lookup_satp_asid (csr_satp_asid),
        .lookup_rsp_vld   (dtlb_rsp_vld),
        .lookup_hit       (dtlb_hit),
        .lookup_pte       (dtlb_pte),
        .lookup_level     (dtlb_level),
        .fill_vld         (fill_tlb_vld && !fill_tlb_is_inst),
        .fill_va          (fill_tlb_va),
        .fill_satp_ppn    (fill_tlb_satp_ppn),
        .fill_satp_asid   (fill_tlb_satp_asid),
        .fill_level       (fill_tlb_level),
        .fill_pte         (fill_tlb_pte)
    );

    always_comb begin
        new_d_bare_issue = d_fifo_rd_vld && !d_translate && !walk_busy &&
            !pend_pa_vld && d_ost_alloc_rdy && pa_d_req_mst.rdy;
        new_i_bare_issue = i_fifo_rd_vld && !i_translate && !walk_busy &&
            !pend_pa_vld && !new_d_bare_issue && i_ost_alloc_rdy &&
            pa_i_req_mst.rdy;
        new_d_lookup_issue = d_fifo_rd_vld && d_translate && !walk_busy &&
            !pend_pa_vld && !lookup_vld_q && d_ost_alloc_rdy;
        new_i_lookup_issue = i_fifo_rd_vld && i_translate && !walk_busy &&
            !pend_pa_vld && !lookup_vld_q && !new_d_lookup_issue &&
            i_ost_alloc_rdy;
    end

    assign itlb_lookup_vld = new_i_lookup_issue;
    assign dtlb_lookup_vld = new_d_lookup_issue;

    always_comb begin
        i_ost_alloc_vld = new_i_bare_issue || new_i_lookup_issue;
        i_ost_alloc_ctx = make_ctx(1'b1, i_req_pkt, new_i_bare_issue);
        d_ost_alloc_vld = new_d_bare_issue || new_d_lookup_issue;
        d_ost_alloc_ctx = make_ctx(1'b0, d_req_pkt, new_d_bare_issue);
    end

    assign i_fifo_rd_rdy = new_i_bare_issue || new_i_lookup_issue;
    assign d_fifo_rd_rdy = new_d_bare_issue || new_d_lookup_issue;

    always_comb begin
        pa_i_from_walk = walk_state == WALK_SEND_FINAL && walk_is_inst;
        pa_d_from_walk_pte = walk_state == WALK_SEND_PTE;
        pa_d_from_walk_final = walk_state == WALK_SEND_FINAL && !walk_is_inst;
        pa_i_from_pend = pend_pa_vld && pend_pa_is_inst && !pa_i_from_walk;
        pa_d_from_pend = pend_pa_vld && !pend_pa_is_inst &&
            !pa_d_from_walk_pte && !pa_d_from_walk_final;
        pa_i_from_new_bare = new_i_bare_issue && !pa_i_from_walk &&
            !pa_i_from_pend;
        pa_d_from_new_bare = new_d_bare_issue && !pa_d_from_walk_pte &&
            !pa_d_from_walk_final && !pa_d_from_pend;
    end

    assign pa_i_req_mst.vld = pa_i_from_walk || pa_i_from_pend ||
        pa_i_from_new_bare;
    assign pa_i_req_mst.pkt =
        pa_i_from_walk ? walk_req :
        pa_i_from_pend ? pend_pa_req : i_req_pkt;

    assign pa_d_req_mst.vld = pa_d_from_walk_pte || pa_d_from_walk_final ||
        pa_d_from_pend || pa_d_from_new_bare;
    assign pa_d_req_mst.pkt =
        pa_d_from_walk_pte ?
            bti_req_pkt_t'{trans_id:MMU_PTE_TRANS_ID, cmd:BTI_REQ_CMD_READ,
                addr:pte_addr(walk_root_base, walk_va, walk_level),
                size:BTI_REQ_SIZE_B4, data:32'h0, strobe:4'hf} :
        pa_d_from_walk_final ? walk_req :
        pa_d_from_pend ? pend_pa_req : d_req_pkt;

    assign pa_i_rsp_slv.rdy = i_wait_found;
    assign pa_d_rsp_slv.rdy = walk_state == WALK_WAIT_PTE ||
        walk_state == WALK_WAIT_FINAL || d_wait_found;

    assign va_i_rsp_mst.vld = i_ost_head_vld && i_ost_head_ctx.ready &&
        !i_ost_head_ctx.expt_vld;
    assign va_i_rsp_mst.pkt = i_ost_head_ctx.rsp;
    assign va_d_rsp_mst.vld = d_ost_head_vld && d_ost_head_ctx.ready;
    assign va_d_rsp_mst.pkt = d_ost_head_ctx.rsp;
    assign i_ost_free_head = va_i_rsp_hsk ||
        (i_ost_head_vld && i_ost_head_ctx.ready && i_ost_head_ctx.expt_vld);
    assign d_ost_free_head = va_d_rsp_hsk;

    assign fch_expt_mst.vld = i_ost_head_vld && i_ost_head_ctx.ready &&
        i_ost_head_ctx.expt_vld;
    assign fch_expt_mst.pkt = i_ost_head_ctx.expt;
    assign ldst_expt_mst.vld = d_ost_head_vld && d_ost_head_ctx.ready &&
        d_ost_head_ctx.expt_vld;
    assign ldst_expt_mst.pkt = d_ost_head_ctx.expt;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            lookup_vld_q <= 1'b0;
            lookup_is_inst_q <= 1'b0;
            lookup_i_slot_q <= '0;
            lookup_d_slot_q <= '0;
            lookup_req_q <= '0;
            lookup_va_q <= '0;
            lookup_pc_q <= '0;
            lookup_priv_q <= 2'b11;
            lookup_sum_q <= 1'b0;
            lookup_mxr_q <= 1'b0;
            lookup_fault_cause_q <= HART_EXPT_CAUSE_INST_PAGE_FAULT;
            pend_pa_vld <= 1'b0;
            pend_pa_is_inst <= 1'b0;
            pend_pa_i_slot <= '0;
            pend_pa_d_slot <= '0;
            pend_pa_req <= '0;
            walk_state <= WALK_IDLE;
            walk_is_inst <= 1'b0;
            walk_i_slot <= '0;
            walk_d_slot <= '0;
            walk_req <= '0;
            walk_va <= '0;
            walk_pc <= '0;
            walk_priv <= 2'b11;
            walk_sum <= 1'b0;
            walk_mxr <= 1'b0;
            walk_root_base <= '0;
            walk_level <= 1'b1;
            walk_pte <= '0;
            walk_satp_ppn <= '0;
            walk_satp_asid <= '0;
            walk_fault_cause <= HART_EXPT_CAUSE_INST_PAGE_FAULT;
            fill_tlb_vld <= 1'b0;
            fill_tlb_is_inst <= 1'b0;
            fill_tlb_va <= '0;
            fill_tlb_satp_ppn <= '0;
            fill_tlb_satp_asid <= '0;
            fill_tlb_level <= 1'b0;
            fill_tlb_pte <= '0;
            i_ost_slot_wr_vld <= 1'b0;
            i_ost_slot_wr_idx <= '0;
            i_ost_slot_wr_ctx <= '0;
            d_ost_slot_wr_vld <= 1'b0;
            d_ost_slot_wr_idx <= '0;
            d_ost_slot_wr_ctx <= '0;
        end else begin
            fill_tlb_vld <= 1'b0;
            i_ost_slot_wr_vld <= 1'b0;
            d_ost_slot_wr_vld <= 1'b0;

            if (pa_i_rsp_hsk && walk_state != WALK_WAIT_FINAL &&
                i_wait_found) begin
                i_ost_slot_wr_vld <= 1'b1;
                i_ost_slot_wr_idx <= i_wait_slot;
                i_ost_slot_wr_ctx <= complete_rsp(i_wait_ctx, pa_i_rsp_slv.pkt);
            end else if (pend_pa_vld && pend_pa_is_inst && pa_i_req_hsk) begin
                i_ost_slot_wr_vld <= 1'b1;
                i_ost_slot_wr_idx <= pend_pa_i_slot;
                i_ost_slot_wr_ctx <= mark_pa_issued(i_slot_ctx[pend_pa_i_slot]);
            end

            if (pa_d_rsp_hsk && walk_state != WALK_WAIT_PTE &&
                walk_state != WALK_WAIT_FINAL && d_wait_found) begin
                d_ost_slot_wr_vld <= 1'b1;
                d_ost_slot_wr_idx <= d_wait_slot;
                d_ost_slot_wr_ctx <= complete_rsp(d_wait_ctx, pa_d_rsp_slv.pkt);
            end else if (pend_pa_vld && !pend_pa_is_inst && pa_d_req_hsk) begin
                d_ost_slot_wr_vld <= 1'b1;
                d_ost_slot_wr_idx <= pend_pa_d_slot;
                d_ost_slot_wr_ctx <= mark_pa_issued(d_slot_ctx[pend_pa_d_slot]);
            end

            if (new_d_lookup_issue) begin
                lookup_vld_q <= 1'b1;
                lookup_is_inst_q <= 1'b0;
                lookup_d_slot_q <= d_ost_alloc_slot;
                lookup_req_q <= d_req_pkt;
                lookup_va_q <= d_req_pkt.addr;
                lookup_pc_q <= exu_state_slv.pkt.pc;
                lookup_priv_q <= data_priv;
                lookup_sum_q <= data_sum;
                lookup_mxr_q <= data_mxr;
                lookup_fault_cause_q <= fault_cause_for(1'b0, d_req_pkt.cmd);
            end else if (new_i_lookup_issue) begin
                lookup_vld_q <= 1'b1;
                lookup_is_inst_q <= 1'b1;
                lookup_i_slot_q <= i_ost_alloc_slot;
                lookup_req_q <= i_req_pkt;
                lookup_va_q <= i_req_pkt.addr;
                lookup_pc_q <= i_req_pkt.addr;
                lookup_priv_q <= exu_state_slv.pkt.priv;
                lookup_sum_q <= data_sum;
                lookup_mxr_q <= data_mxr;
                lookup_fault_cause_q <= HART_EXPT_CAUSE_INST_PAGE_FAULT;
            end else if ((lookup_is_inst_q && itlb_rsp_vld) ||
                (!lookup_is_inst_q && dtlb_rsp_vld)) begin
                lookup_vld_q <= 1'b0;
                if (lookup_is_inst_q ? itlb_hit : dtlb_hit) begin
                    logic [31:0] hit_pte;
                    logic hit_level;
                    ost_ctx_t ctx;
                    hit_pte = lookup_is_inst_q ? itlb_pte : dtlb_pte;
                    hit_level = lookup_is_inst_q ? itlb_level : dtlb_level;
                    if (!pte_permits(hit_pte, lookup_is_inst_q, lookup_req_q.cmd,
                        lookup_priv_q, lookup_sum_q, lookup_mxr_q) ||
                        !leaf_pa_ok(hit_pte, hit_level)) begin
                        if (lookup_is_inst_q) begin
                            ctx = complete_fault(i_slot_ctx[lookup_i_slot_q],
                                lookup_fault_cause_q, lookup_priv_q,
                                lookup_pc_q, lookup_va_q);
                            i_ost_slot_wr_vld <= 1'b1;
                            i_ost_slot_wr_idx <= lookup_i_slot_q;
                            i_ost_slot_wr_ctx <= ctx;
                        end else begin
                            ctx = complete_fault(d_slot_ctx[lookup_d_slot_q],
                                lookup_fault_cause_q, lookup_priv_q,
                                lookup_pc_q, lookup_va_q);
                            d_ost_slot_wr_vld <= 1'b1;
                            d_ost_slot_wr_idx <= lookup_d_slot_q;
                            d_ost_slot_wr_ctx <= ctx;
                        end
                    end else begin
                        pend_pa_vld <= 1'b1;
                        pend_pa_is_inst <= lookup_is_inst_q;
                        pend_pa_i_slot <= lookup_i_slot_q;
                        pend_pa_d_slot <= lookup_d_slot_q;
                        pend_pa_req <= lookup_req_q;
                        pend_pa_req.addr <= leaf_pa(hit_pte, lookup_va_q, hit_level);
                    end
                end else begin
                    walk_state <= WALK_WAIT_OST_DRAIN;
                    walk_is_inst <= lookup_is_inst_q;
                    walk_i_slot <= lookup_i_slot_q;
                    walk_d_slot <= lookup_d_slot_q;
                    walk_req <= lookup_req_q;
                    walk_va <= lookup_va_q;
                    walk_pc <= lookup_pc_q;
                    walk_priv <= lookup_priv_q;
                    walk_sum <= lookup_sum_q;
                    walk_mxr <= lookup_mxr_q;
                    walk_root_base <= {csr_satp_ppn[19:0], 12'b0};
                    walk_level <= 1'b1;
                    walk_satp_ppn <= csr_satp_ppn;
                    walk_satp_asid <= csr_satp_asid;
                    walk_fault_cause <= lookup_fault_cause_q;
                end
            end

            if (pend_pa_vld &&
                ((pend_pa_is_inst && pa_i_req_hsk) ||
                (!pend_pa_is_inst && pa_d_req_hsk))) begin
                pend_pa_vld <= 1'b0;
            end

            unique case (walk_state)
            WALK_IDLE: ;
            WALK_WAIT_OST_DRAIN: begin
                if ((walk_is_inst && i_ost_head_vld &&
                    i_ost_head_slot == walk_i_slot && !(|d_ost_slot_vld)) ||
                    (!walk_is_inst && d_ost_head_vld &&
                    d_ost_head_slot == walk_d_slot && !(|i_ost_slot_vld))) begin
                    walk_state <= WALK_SEND_PTE;
                end
            end
            WALK_SEND_PTE: begin
                if (pa_d_req_hsk)
                    walk_state <= WALK_WAIT_PTE;
            end
            WALK_WAIT_PTE: begin
                if (pa_d_rsp_hsk) begin
                    walk_pte <= pa_d_rsp_slv.pkt.data;
                    if (!pa_d_rsp_slv.pkt.ok || !pte_valid(pa_d_rsp_slv.pkt.data)) begin
                        if (walk_is_inst) begin
                            i_ost_slot_wr_vld <= 1'b1;
                            i_ost_slot_wr_idx <= walk_i_slot;
                            i_ost_slot_wr_ctx <= complete_fault(i_slot_ctx[walk_i_slot],
                                walk_fault_cause, walk_priv, walk_pc, walk_va);
                        end else begin
                            d_ost_slot_wr_vld <= 1'b1;
                            d_ost_slot_wr_idx <= walk_d_slot;
                            d_ost_slot_wr_ctx <= complete_fault(d_slot_ctx[walk_d_slot],
                                walk_fault_cause, walk_priv, walk_pc, walk_va);
                        end
                        walk_state <= WALK_IDLE;
                    end else if (!pte_leaf(pa_d_rsp_slv.pkt.data)) begin
                        if (!walk_level || |pa_d_rsp_slv.pkt.data[7:4]) begin
                            if (walk_is_inst) begin
                                i_ost_slot_wr_vld <= 1'b1;
                                i_ost_slot_wr_idx <= walk_i_slot;
                                i_ost_slot_wr_ctx <= complete_fault(i_slot_ctx[walk_i_slot],
                                    walk_fault_cause, walk_priv, walk_pc, walk_va);
                            end else begin
                                d_ost_slot_wr_vld <= 1'b1;
                                d_ost_slot_wr_idx <= walk_d_slot;
                                d_ost_slot_wr_ctx <= complete_fault(d_slot_ctx[walk_d_slot],
                                    walk_fault_cause, walk_priv, walk_pc, walk_va);
                            end
                            walk_state <= WALK_IDLE;
                        end else begin
                            walk_root_base <= {pa_d_rsp_slv.pkt.data[29:10], 12'b0};
                            walk_level <= 1'b0;
                            walk_state <= WALK_SEND_PTE;
                        end
                    end else if (!pte_permits(pa_d_rsp_slv.pkt.data, walk_is_inst,
                        walk_req.cmd, walk_priv, walk_sum, walk_mxr) ||
                        !leaf_pa_ok(pa_d_rsp_slv.pkt.data, walk_level)) begin
                        if (walk_is_inst) begin
                            i_ost_slot_wr_vld <= 1'b1;
                            i_ost_slot_wr_idx <= walk_i_slot;
                            i_ost_slot_wr_ctx <= complete_fault(i_slot_ctx[walk_i_slot],
                                walk_fault_cause, walk_priv, walk_pc, walk_va);
                        end else begin
                            d_ost_slot_wr_vld <= 1'b1;
                            d_ost_slot_wr_idx <= walk_d_slot;
                            d_ost_slot_wr_ctx <= complete_fault(d_slot_ctx[walk_d_slot],
                                walk_fault_cause, walk_priv, walk_pc, walk_va);
                        end
                        walk_state <= WALK_IDLE;
                    end else begin
                        walk_req.addr <= leaf_pa(pa_d_rsp_slv.pkt.data, walk_va,
                            walk_level);
                        fill_tlb_vld <= 1'b1;
                        fill_tlb_is_inst <= walk_is_inst;
                        fill_tlb_va <= walk_va;
                        fill_tlb_satp_ppn <= walk_satp_ppn;
                        fill_tlb_satp_asid <= walk_satp_asid;
                        fill_tlb_level <= walk_level;
                        fill_tlb_pte <= pa_d_rsp_slv.pkt.data;
                        walk_state <= WALK_SEND_FINAL;
                    end
                end
            end
            WALK_SEND_FINAL: begin
                if (walk_is_inst ? pa_i_req_hsk : pa_d_req_hsk) begin
                    if (walk_is_inst) begin
                        i_ost_slot_wr_vld <= 1'b1;
                        i_ost_slot_wr_idx <= walk_i_slot;
                        i_ost_slot_wr_ctx <= mark_pa_issued(i_slot_ctx[walk_i_slot]);
                    end else begin
                        d_ost_slot_wr_vld <= 1'b1;
                        d_ost_slot_wr_idx <= walk_d_slot;
                        d_ost_slot_wr_ctx <= mark_pa_issued(d_slot_ctx[walk_d_slot]);
                    end
                    walk_state <= WALK_WAIT_FINAL;
                end
            end
            WALK_WAIT_FINAL: begin
                if (walk_is_inst ? pa_i_rsp_hsk : pa_d_rsp_hsk) begin
                    if (walk_is_inst) begin
                        i_ost_slot_wr_vld <= 1'b1;
                        i_ost_slot_wr_idx <= walk_i_slot;
                        i_ost_slot_wr_ctx <= complete_rsp(i_slot_ctx[walk_i_slot],
                            pa_i_rsp_slv.pkt);
                    end else begin
                        d_ost_slot_wr_vld <= 1'b1;
                        d_ost_slot_wr_idx <= walk_d_slot;
                        d_ost_slot_wr_ctx <= complete_rsp(d_slot_ctx[walk_d_slot],
                            pa_d_rsp_slv.pkt);
                    end
                    walk_state <= WALK_IDLE;
                end
            end
            default: walk_state <= WALK_IDLE;
            endcase
        end
    end

`ifndef SYNTHESIS
    logic rtl_progress_en;
    longint unsigned rtl_progress_cycle;

    initial begin
        rtl_progress_en = $test$plusargs("rtl_mmu_progress");
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rtl_progress_cycle <= 0;
        end else if (rtl_progress_en) begin
            rtl_progress_cycle <= rtl_progress_cycle + 1;
            if (rtl_progress_cycle[19:0] == 20'h0 ||
                tlb_flush ||
                (va_i_req_slv.vld && va_i_req_slv.rdy) ||
                (va_d_req_slv.vld && va_d_req_slv.rdy) ||
                va_i_rsp_hsk || va_d_rsp_hsk ||
                pa_i_req_hsk || pa_d_req_hsk ||
                pa_i_rsp_hsk || pa_d_rsp_hsk ||
                fch_expt_mst.vld || ldst_expt_mst.vld) begin
                $display("[RTL_PROGRESS][%m] cycle=%0d satp=%08x priv=%0d dpriv=%0d walk=%0d wva=%08x wi=%0d pend=%0d/%0d iost=%b dost=%b ih=%0d dh=%0d vi=%0b/%0b:%0b/%0b vd=%0b/%0b:%0b/%0b pi=%0b/%0b:%0b/%0b pd=%0b/%0b:%0b/%0b ex=%0b/%0b",
                    rtl_progress_cycle,
                    csr_mmu_state_slv.pkt.satp,
                    exu_state_slv.pkt.priv,
                    data_priv,
                    walk_state,
                    walk_va,
                    walk_is_inst,
                    pend_pa_vld,
                    pend_pa_is_inst,
                    i_ost_slot_vld,
                    d_ost_slot_vld,
                    i_ost_head_vld,
                    d_ost_head_vld,
                    va_i_req_slv.vld, va_i_req_slv.rdy,
                    va_i_rsp_mst.vld, va_i_rsp_mst.rdy,
                    va_d_req_slv.vld, va_d_req_slv.rdy,
                    va_d_rsp_mst.vld, va_d_rsp_mst.rdy,
                    pa_i_req_mst.vld, pa_i_req_mst.rdy,
                    pa_i_rsp_slv.vld, pa_i_rsp_slv.rdy,
                    pa_d_req_mst.vld, pa_d_req_mst.rdy,
                    pa_d_rsp_slv.vld, pa_d_rsp_slv.rdy,
                    fch_expt_mst.vld,
                    ldst_expt_mst.vld);
            end
        end
    end
`endif
endmodule
