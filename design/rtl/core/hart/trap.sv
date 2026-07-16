`include "itf/hart_expt_if.svh"

module trap(
    input logic clk,
    input logic rst_n,
    hart_expt_if_t.slv ex_expt_slv,
    hart_expt_if_t.slv fch_expt_slv,
    hart_expt_if_t.slv ldst_expt_slv,
    exu_state_if_t.slv exu_state_slv,
    trap_exu_ctrl_if_t.mst trap_exu_ctrl_mst,
    csr_trap_state_if_t.slv csr_trap_state_slv,
    trap_csr_write_req_if_t.mst trap_csr_write_req_mst,
    csr_trap_write_rsp_if_t.slv csr_trap_write_rsp_slv,
    trap_send_if_t.mst trap_send_mst
);
    typedef enum logic [1:0] {IDLE, PREPARE, WRITE, REDIRECT} state_t;
    state_t state;
    logic [1:0] write_idx;
    logic [2:0] write_count;
    logic [11:0] wr_addr[0:3];
    logic [31:0] wr_val[0:3];
    logic [31:0] target_pc;
    logic [1:0] target_priv;
    logic event_mret_q;
    logic event_sret_q;
    logic event_is_interrupt_q;
    logic event_delegated_q;
    logic [1:0] event_source_priv_q;
    logic [4:0] event_cause_q;
    logic [31:0] event_epc_q;
    logic [31:0] event_tval_q;
    logic [31:0] event_mstatus_q;
    logic [31:0] event_mepc_q;
    logic [31:0] event_sepc_q;
    logic [31:0] event_mtvec_q;
    logic [31:0] event_stvec_q;

    wire [31:0] pending = csr_trap_state_slv.pkt.mip & csr_trap_state_slv.pkt.mie;
    wire [31:0] m_pending = pending & ~csr_trap_state_slv.pkt.mideleg;
    wire [31:0] s_pending = pending & csr_trap_state_slv.pkt.mideleg;
    wire m_irq_enabled = exu_state_slv.pkt.priv != 2'b11 || csr_trap_state_slv.pkt.mstatus[3];
    wire s_irq_enabled = exu_state_slv.pkt.priv == 2'b00 ||
        (exu_state_slv.pkt.priv == 2'b01 && csr_trap_state_slv.pkt.mstatus[1]);
    wire m_irq_valid = m_irq_enabled && |m_pending;
    wire s_irq_valid = s_irq_enabled && |s_pending;
    wire irq_valid = !exu_state_slv.pkt.irq_defer && (m_irq_valid || s_irq_valid);
    wire wfi_wakeup = state == IDLE && exu_state_slv.pkt.wfi && |pending && !irq_valid;
    wire irq_to_s = !m_irq_valid && s_irq_valid;
    wire [31:0] selected_pending = m_irq_valid ? m_pending : s_pending;
    logic [4:0] irq_cause;
    always_comb begin
        if (selected_pending[11]) irq_cause = 5'd11;
        else if (selected_pending[3]) irq_cause = 5'd3;
        else if (selected_pending[7]) irq_cause = 5'd7;
        else if (selected_pending[9]) irq_cause = 5'd9;
        else if (selected_pending[1]) irq_cause = 5'd1;
        else irq_cause = 5'd5;
    end

    wire expt_valid = ex_expt_slv.vld || fch_expt_slv.vld || ldst_expt_slv.vld;
    wire event_valid = expt_valid || irq_valid;
    wire event_mret = ex_expt_slv.vld && ex_expt_slv.pkt.expt_type == HART_EXPT_TYPE_MRET;
    wire event_sret = ex_expt_slv.vld && ex_expt_slv.pkt.expt_type == HART_EXPT_TYPE_SRET;
    wire [1:0] expt_priv = ex_expt_slv.vld ? ex_expt_slv.pkt.priv :
        (fch_expt_slv.vld ? fch_expt_slv.pkt.priv : ldst_expt_slv.pkt.priv);
    wire [4:0] expt_cause = ex_expt_slv.vld ? ex_expt_slv.pkt.cause :
        (fch_expt_slv.vld ? fch_expt_slv.pkt.cause : ldst_expt_slv.pkt.cause);
    wire [31:0] expt_pc = ex_expt_slv.vld ? ex_expt_slv.pkt.pc :
        (fch_expt_slv.vld ? fch_expt_slv.pkt.pc : ldst_expt_slv.pkt.pc);
    wire [31:0] expt_tval = ex_expt_slv.vld ? ex_expt_slv.pkt.tval :
        (fch_expt_slv.vld ? fch_expt_slv.pkt.tval : ldst_expt_slv.pkt.tval);
    wire event_is_interrupt = irq_valid && !expt_valid;
    wire [1:0] event_source_priv = event_is_interrupt ?
        exu_state_slv.pkt.priv : expt_priv;
    wire [4:0] event_cause = event_is_interrupt ? irq_cause :
        (expt_cause == HART_EXPT_CAUSE_ECALL ?
        (expt_priv == 2'b11 ? 5'd11 : (expt_priv == 2'b01 ? 5'd9 : 5'd8)) :
        expt_cause);
    wire event_delegated = event_is_interrupt ? irq_to_s :
        (expt_priv != 2'b11 && csr_trap_state_slv.pkt.medeleg[event_cause]);
    wire [31:0] event_epc = event_is_interrupt ?
        (exu_state_slv.pkt.wfi ? exu_state_slv.pkt.wfi_resume_pc :
        exu_state_slv.pkt.irq_epc) : expt_pc;

    integer i;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            write_idx <= 0;
            write_count <= 0;
            target_pc <= 0;
            target_priv <= 2'b11;
            event_mret_q <= 1'b0;
            event_sret_q <= 1'b0;
            event_is_interrupt_q <= 1'b0;
            event_delegated_q <= 1'b0;
            event_source_priv_q <= 2'b11;
            event_cause_q <= '0;
            event_epc_q <= '0;
            event_tval_q <= '0;
            event_mstatus_q <= '0;
            event_mepc_q <= '0;
            event_sepc_q <= '0;
            event_mtvec_q <= '0;
            event_stvec_q <= '0;
            for (i = 0; i < 4; i = i + 1) begin wr_addr[i] <= 0; wr_val[i] <= 0; end
        end else begin
            case (state)
                IDLE: if (event_valid) begin
                    event_mret_q <= event_mret;
                    event_sret_q <= event_sret;
                    event_is_interrupt_q <= event_is_interrupt;
                    event_delegated_q <= event_delegated;
                    event_source_priv_q <= event_source_priv;
                    event_cause_q <= event_cause;
                    event_epc_q <= event_epc;
                    event_tval_q <= expt_valid ? expt_tval : 32'b0;
                    event_mstatus_q <= csr_trap_state_slv.pkt.mstatus;
                    event_mepc_q <= csr_trap_state_slv.pkt.mepc;
                    event_sepc_q <= csr_trap_state_slv.pkt.sepc;
                    event_mtvec_q <= csr_trap_state_slv.pkt.mtvec;
                    event_stvec_q <= csr_trap_state_slv.pkt.stvec;
                    state <= PREPARE;
                end
                PREPARE: begin
                    write_idx <= 0;
                    if (event_mret_q) begin
                        wr_addr[0] <= 12'h300;
                        wr_val[0] <= ((event_mstatus_q & ~32'h00001888) |
                            (event_mstatus_q[7] ? 32'h00000008 : 32'b0) |
                            32'h00000080) &
                            (event_mstatus_q[12:11] == 2'b11 ?
                            32'hffffffff : ~32'h00020000);
                        write_count <= 1;
                        target_pc <= event_mepc_q & 32'hfffffffc;
                        target_priv <= event_mstatus_q[12:11];
                    end else if (event_sret_q) begin
                        wr_addr[0] <= 12'h300;
                        wr_val[0] <= (event_mstatus_q & ~32'h00020122) |
                            (event_mstatus_q[5] ? 32'h00000002 : 32'b0) |
                            32'h00000020;
                        write_count <= 1;
                        target_pc <= event_sepc_q & 32'hfffffffc;
                        target_priv <= event_mstatus_q[8] ? 2'b01 : 2'b00;
                    end else begin : take_trap
                        wr_addr[0] <= 12'h300;
                        wr_val[0] <= event_delegated_q ?
                            ((event_mstatus_q & ~32'h00000122) |
                            (event_mstatus_q[1] ? 32'h00000020 : 32'b0) |
                            (event_source_priv_q == 2'b01 ? 32'h00000100 : 32'b0)) :
                            ((event_mstatus_q & ~32'h00001888) |
                            (event_mstatus_q[3] ? 32'h00000080 : 32'b0) |
                            {19'b0, event_source_priv_q, 11'b0});
                        wr_addr[1] <= event_delegated_q ? 12'h142 : 12'h342;
                        wr_val[1] <= {event_is_interrupt_q, 26'b0, event_cause_q};
                        wr_addr[2] <= event_delegated_q ? 12'h141 : 12'h341;
                        wr_val[2] <= event_epc_q & 32'hfffffffc;
                        wr_addr[3] <= event_delegated_q ? 12'h143 : 12'h343;
                        wr_val[3] <= event_tval_q;
                        write_count <= 4;
                        target_pc <= (event_delegated_q ?
                            {event_stvec_q[31:2], 2'b00} :
                            {event_mtvec_q[31:2], 2'b00}) +
                            ((event_is_interrupt_q &&
                            (event_delegated_q ? event_stvec_q[1:0] == 2'b01 :
                            event_mtvec_q[1:0] == 2'b01)) ?
                            {25'b0, event_cause_q, 2'b00} : 32'b0);
                        target_priv <= event_delegated_q ? 2'b01 : 2'b11;
                    end
                    state <= WRITE;
                end
                WRITE: begin
                    if (csr_trap_write_rsp_slv.vld && csr_trap_write_rsp_slv.pkt.ok) begin
                        if (write_idx + 1 >= write_count) state <= REDIRECT;
                        else write_idx <= write_idx + 1'b1;
                    end
                end
                REDIRECT: state <= IDLE;
                default: state <= IDLE;
            endcase
        end
    end

    assign trap_csr_write_req_mst.vld = state == WRITE;
    assign trap_csr_write_req_mst.pkt.addr = wr_addr[write_idx];
    assign trap_csr_write_req_mst.pkt.val = wr_val[write_idx];
    assign trap_send_mst.vld = state == REDIRECT;
    assign trap_send_mst.pkt.target_pc = target_pc;
    assign trap_exu_ctrl_mst.vld = (state == IDLE && (event_valid || wfi_wakeup)) || state == REDIRECT;
    assign trap_exu_ctrl_mst.pkt.priv = state == REDIRECT ? target_priv : exu_state_slv.pkt.priv;
    assign trap_exu_ctrl_mst.pkt.irq_epc = wfi_wakeup ? exu_state_slv.pkt.wfi_resume_pc : target_pc;
    assign trap_exu_ctrl_mst.pkt.wfi = state != REDIRECT && !wfi_wakeup;

endmodule
