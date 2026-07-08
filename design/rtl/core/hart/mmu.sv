`include "itf/bti_req_if.svh"
`include "itf/hart_expt_if.svh"

module mmu(
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
    hart_expt_if_t.mst fch_expt_mst,
    hart_expt_if_t.mst ldst_expt_mst
);
    typedef enum logic [3:0] {
        IDLE, SEND_BARE, WAIT_BARE, SEND_PTE, WAIT_PTE,
        SEND_FINAL, WAIT_FINAL, FAULT_EVENT, FAULT_RSP
    } state_t;
    typedef struct packed {
        logic [15:0] trans_id;
        bti_req_cmd_t cmd;
        logic [31:0] addr;
        bti_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } bti_req_pkt_t;

    state_t state;
    bti_req_pkt_t req;
    logic is_inst;
    logic [1:0] req_priv;
    logic req_sum;
    logic req_mxr;
    logic [31:0] va;
    logic [31:0] fault_pc;
    logic [31:0] root_base;
    logic [31:0] last_pte;
    logic level;
    hart_expt_cause_t fault_cause;

    wire data_write = va_d_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE;
    wire [1:0] data_priv = exu_state_slv.pkt.priv == 2'b11 &&
        csr_mmu_state_slv.pkt.mstatus[17] ? csr_mmu_state_slv.pkt.mstatus[12:11] :
        exu_state_slv.pkt.priv;
    wire i_translate = exu_state_slv.pkt.priv != 2'b11 && csr_mmu_state_slv.pkt.satp[31];
    wire d_translate = data_priv != 2'b11 && csr_mmu_state_slv.pkt.satp[31];
    wire accept_d = state == IDLE && va_d_req_slv.vld;
    wire accept_i = state == IDLE && !va_d_req_slv.vld && va_i_req_slv.vld;

    wire use_d_req = state == SEND_PTE ||
        ((state == SEND_BARE || state == SEND_FINAL) && !is_inst);
    wire use_i_req = (state == SEND_BARE || state == SEND_FINAL) && is_inst;
    wire d_req_hsk = pa_d_req_mst.vld && pa_d_req_mst.rdy;
    wire i_req_hsk = pa_i_req_mst.vld && pa_i_req_mst.rdy;
    wire d_rsp_hsk = pa_d_rsp_slv.vld && pa_d_rsp_slv.rdy;
    wire i_rsp_hsk = pa_i_rsp_slv.vld && pa_i_rsp_slv.rdy;
    wire fault_rsp_hsk = is_inst ? (va_i_rsp_mst.vld && va_i_rsp_mst.rdy) :
        (va_d_rsp_mst.vld && va_d_rsp_mst.rdy);

    function automatic logic pte_valid(input logic [31:0] pte);
        pte_valid = pte[0] && !(pte[2] && !pte[1]);
    endfunction

    function automatic logic pte_leaf(input logic [31:0] pte);
        pte_leaf = |pte[3:1];
    endfunction

    function automatic logic pte_permits(input logic [31:0] pte);
        logic user_ok;
        logic access_ok;
        begin
            user_ok = 1'b1;
            if (req_priv == 2'b00 && !pte[4]) user_ok = 1'b0;
            if (req_priv == 2'b01 && pte[4] && (is_inst || !req_sum)) user_ok = 1'b0;
            if (is_inst) access_ok = pte[3];
            else if (req.cmd == BTI_REQ_CMD_WRITE) access_ok = pte[2];
            else access_ok = pte[1] || (req_mxr && pte[3]);
            pte_permits = user_ok && access_ok && pte[6] &&
                (is_inst || req.cmd != BTI_REQ_CMD_WRITE || pte[7]);
        end
    endfunction

    function automatic logic [31:0] pte_addr;
        logic [9:0] vpn;
        begin
            vpn = level ? va[31:22] : va[21:12];
            pte_addr = root_base + {20'b0, vpn, 2'b00};
        end
    endfunction

    logic [31:0] translated_addr;
    logic leaf_addr_ok;
    always_comb begin
        leaf_addr_ok = 1'b1;
        if (level) begin
            leaf_addr_ok = pa_d_rsp_slv.pkt.data[19:10] == 0;
            translated_addr = {pa_d_rsp_slv.pkt.data[29:20], va[21:0]};
        end else begin
            translated_addr = {pa_d_rsp_slv.pkt.data[29:10], va[11:0]};
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            req <= '0;
            is_inst <= 1'b0;
            req_priv <= 2'b11;
            req_sum <= 1'b0;
            req_mxr <= 1'b0;
            va <= 0;
            fault_pc <= 0;
            root_base <= 0;
            last_pte <= 0;
            level <= 1'b1;
            fault_cause <= HART_EXPT_CAUSE_INST_PAGE_FAULT;
        end else begin
            case (state)
                IDLE: begin
                    if (accept_d) begin
                        req <= va_d_req_slv.pkt;
                        is_inst <= 1'b0;
                        req_priv <= data_priv;
                        req_sum <= csr_mmu_state_slv.pkt.mstatus[18];
                        req_mxr <= csr_mmu_state_slv.pkt.mstatus[19];
                        va <= va_d_req_slv.pkt.addr;
                        fault_pc <= exu_state_slv.pkt.pc;
                        root_base <= {csr_mmu_state_slv.pkt.satp[19:0], 12'b0};
                        level <= 1'b1;
                        fault_cause <= data_write ? HART_EXPT_CAUSE_STORE_AMO_PAGE_FAULT :
                            HART_EXPT_CAUSE_LOAD_PAGE_FAULT;
                        state <= d_translate ? SEND_PTE : SEND_BARE;
                    end else if (accept_i) begin
                        req <= va_i_req_slv.pkt;
                        is_inst <= 1'b1;
                        req_priv <= exu_state_slv.pkt.priv;
                        req_sum <= csr_mmu_state_slv.pkt.mstatus[18];
                        req_mxr <= csr_mmu_state_slv.pkt.mstatus[19];
                        va <= va_i_req_slv.pkt.addr;
                        fault_pc <= va_i_req_slv.pkt.addr;
                        root_base <= {csr_mmu_state_slv.pkt.satp[19:0], 12'b0};
                        level <= 1'b1;
                        fault_cause <= HART_EXPT_CAUSE_INST_PAGE_FAULT;
                        state <= i_translate ? SEND_PTE : SEND_BARE;
                    end
                end
                SEND_BARE: if (is_inst ? i_req_hsk : d_req_hsk) state <= WAIT_BARE;
                WAIT_BARE: if (is_inst ? i_rsp_hsk : d_rsp_hsk) state <= IDLE;
                SEND_PTE: if (d_req_hsk) state <= WAIT_PTE;
                WAIT_PTE: if (d_rsp_hsk) begin
                    last_pte <= pa_d_rsp_slv.pkt.data;
                    if (!pa_d_rsp_slv.pkt.ok || !pte_valid(pa_d_rsp_slv.pkt.data)) begin
                        state <= FAULT_EVENT;
                    end else if (!pte_leaf(pa_d_rsp_slv.pkt.data)) begin
                        if (!level || |pa_d_rsp_slv.pkt.data[7:4]) begin
                            state <= FAULT_EVENT;
                        end else begin
                            root_base <= {pa_d_rsp_slv.pkt.data[29:10], 12'b0};
                            level <= 1'b0;
                            state <= SEND_PTE;
                        end
                    end else if (!pte_permits(pa_d_rsp_slv.pkt.data) || !leaf_addr_ok) begin
                        state <= FAULT_EVENT;
                    end else begin
                        req.addr <= translated_addr;
                        state <= SEND_FINAL;
                    end
                end
                SEND_FINAL: if (is_inst ? i_req_hsk : d_req_hsk) state <= WAIT_FINAL;
                WAIT_FINAL: if (is_inst ? i_rsp_hsk : d_rsp_hsk) state <= IDLE;
                FAULT_EVENT: state <= FAULT_RSP;
                FAULT_RSP: if (fault_rsp_hsk) state <= IDLE;
                default: state <= IDLE;
            endcase
        end
    end

    assign va_d_req_slv.rdy = accept_d;
    assign va_i_req_slv.rdy = accept_i;

    assign pa_i_req_mst.vld = use_i_req;
    assign pa_i_req_mst.pkt = req;
    assign pa_d_req_mst.vld = use_d_req;
    assign pa_d_req_mst.pkt = state == SEND_PTE ?
        '{trans_id:16'hfffe, cmd:BTI_REQ_CMD_READ, addr:pte_addr(),
          size:BTI_REQ_SIZE_B4, data:32'b0, strobe:4'hf} : req;

    assign pa_i_rsp_slv.rdy = (state == WAIT_BARE || state == WAIT_FINAL) && is_inst ?
        va_i_rsp_mst.rdy : 1'b0;
    assign pa_d_rsp_slv.rdy = state == WAIT_PTE ? 1'b1 :
        (((state == WAIT_BARE || state == WAIT_FINAL) && !is_inst) ? va_d_rsp_mst.rdy : 1'b0);

    assign va_i_rsp_mst.vld = ((state == WAIT_BARE || state == WAIT_FINAL) && is_inst) ?
        pa_i_rsp_slv.vld : (state == FAULT_RSP && is_inst);
    assign va_i_rsp_mst.pkt = state == FAULT_RSP ?
        '{trans_id:req.trans_id, data:32'b0, ok:1'b0} : pa_i_rsp_slv.pkt;
    assign va_d_rsp_mst.vld = ((state == WAIT_BARE || state == WAIT_FINAL) && !is_inst) ?
        pa_d_rsp_slv.vld : (state == FAULT_RSP && !is_inst);
    assign va_d_rsp_mst.pkt = state == FAULT_RSP ?
        '{trans_id:req.trans_id, data:32'b0, ok:1'b0} : pa_d_rsp_slv.pkt;

    assign fch_expt_mst.vld = state == FAULT_EVENT && is_inst;
    assign ldst_expt_mst.vld = state == FAULT_EVENT && !is_inst;
    assign fch_expt_mst.pkt.expt_type = HART_EXPT_TYPE_EXCEPTION;
    assign ldst_expt_mst.pkt.expt_type = HART_EXPT_TYPE_EXCEPTION;
    assign fch_expt_mst.pkt.cause = fault_cause;
    assign ldst_expt_mst.pkt.cause = fault_cause;
    assign fch_expt_mst.pkt.priv = req_priv;
    assign ldst_expt_mst.pkt.priv = req_priv;
    assign fch_expt_mst.pkt.pc = fault_pc;
    assign ldst_expt_mst.pkt.pc = fault_pc;
    assign fch_expt_mst.pkt.tval = va;
    assign ldst_expt_mst.pkt.tval = va;
endmodule
