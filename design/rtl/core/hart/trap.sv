module trap(
    input logic clk,
    input logic rst_n,
    hart_expt_if_t.slv ex_expt_slv,
    exu_state_if_t.slv exu_state_slv,
    trap_exu_ctrl_if_t.mst trap_exu_ctrl_mst,
    csr_trap_state_if_t.slv csr_trap_state_slv,
    trap_csr_write_req_if_t.mst trap_csr_write_req_mst,
    csr_trap_write_rsp_if_t.slv csr_trap_write_rsp_slv,
    trap_send_if_t.mst trap_send_mst
);
    typedef enum logic [1:0] {IDLE, WRITE, REDIRECT} state_t;
    state_t state;
    logic [1:0] write_idx;
    logic [2:0] write_count;
    logic [11:0] wr_addr[0:3];
    logic [31:0] wr_val[0:3];
    logic [31:0] target_pc;
    logic [1:0] target_priv;

    wire [31:0] pending = csr_trap_state_slv.pkt.mip & csr_trap_state_slv.pkt.mie;
    wire [31:0] m_pending = pending & ~csr_trap_state_slv.pkt.mideleg;
    wire [31:0] s_pending = pending & csr_trap_state_slv.pkt.mideleg;
    wire m_irq_enabled = exu_state_slv.pkt.priv != 2'b11 || csr_trap_state_slv.pkt.mstatus[3];
    wire s_irq_enabled = exu_state_slv.pkt.priv == 2'b00 ||
        (exu_state_slv.pkt.priv == 2'b01 && csr_trap_state_slv.pkt.mstatus[1]);
    wire m_irq_valid = m_irq_enabled && |m_pending;
    wire s_irq_valid = s_irq_enabled && |s_pending;
    wire irq_valid = !exu_state_slv.pkt.irq_defer && (m_irq_valid || s_irq_valid);
    wire irq_to_s = !m_irq_valid && s_irq_valid;
    wire [31:0] selected_pending = m_irq_valid ? m_pending : s_pending;
    logic [4:0] irq_cause;
    always_comb begin
        if (selected_pending[11]) irq_cause = 5'd11;
        else if (selected_pending[3]) irq_cause = 5'd3;
        else irq_cause = 5'd7;
    end

    wire event_valid = ex_expt_slv.vld || irq_valid;
    wire event_mret = ex_expt_slv.vld && ex_expt_slv.pkt.expt_type == HART_EXPT_TYPE_MRET;
    wire event_sret = ex_expt_slv.vld && ex_expt_slv.pkt.expt_type == HART_EXPT_TYPE_SRET;

    integer i;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            write_idx <= 0;
            write_count <= 0;
            target_pc <= 0;
            target_priv <= 2'b11;
            for (i = 0; i < 4; i = i + 1) begin wr_addr[i] <= 0; wr_val[i] <= 0; end
        end else begin
            case (state)
                IDLE: if (event_valid) begin
                    write_idx <= 0;
                    if (event_mret) begin
                        wr_addr[0] <= 12'h300;
                        wr_val[0] <= ((csr_trap_state_slv.pkt.mstatus & ~32'h00001888) |
                            (csr_trap_state_slv.pkt.mstatus[7] ? 32'h00000008 : 32'b0) |
                            32'h00000080) &
                            (csr_trap_state_slv.pkt.mstatus[12:11] == 2'b11 ?
                            32'hffffffff : ~32'h00020000);
                        write_count <= 1;
                        target_pc <= csr_trap_state_slv.pkt.mepc & 32'hfffffffc;
                        target_priv <= csr_trap_state_slv.pkt.mstatus[12:11];
                    end else if (event_sret) begin
                        wr_addr[0] <= 12'h300;
                        wr_val[0] <= (csr_trap_state_slv.pkt.mstatus & ~32'h00020122) |
                            (csr_trap_state_slv.pkt.mstatus[5] ? 32'h00000002 : 32'b0) |
                            32'h00000020;
                        write_count <= 1;
                        target_pc <= csr_trap_state_slv.pkt.sepc & 32'hfffffffc;
                        target_priv <= csr_trap_state_slv.pkt.mstatus[8] ? 2'b01 : 2'b00;
                    end else begin : take_trap
                        logic [4:0] cause;
                        logic is_interrupt;
                        logic delegated;
                        logic [31:0] epc;
                        is_interrupt = irq_valid && !ex_expt_slv.vld;
                        cause = is_interrupt ? irq_cause :
                            (exu_state_slv.pkt.priv == 2'b11 ? 5'd11 :
                            (exu_state_slv.pkt.priv == 2'b01 ? 5'd9 : 5'd8));
                        delegated = is_interrupt ? irq_to_s :
                            (exu_state_slv.pkt.priv != 2'b11 && csr_trap_state_slv.pkt.medeleg[cause]);
                        epc = is_interrupt ?
                            (exu_state_slv.pkt.wfi ? exu_state_slv.pkt.wfi_resume_pc : exu_state_slv.pkt.irq_epc) :
                            ex_expt_slv.pkt.pc;
                        wr_addr[0] <= 12'h300;
                        wr_val[0] <= delegated ?
                            ((csr_trap_state_slv.pkt.mstatus & ~32'h00000122) |
                            (csr_trap_state_slv.pkt.mstatus[1] ? 32'h00000020 : 32'b0) |
                            (exu_state_slv.pkt.priv == 2'b01 ? 32'h00000100 : 32'b0)) :
                            ((csr_trap_state_slv.pkt.mstatus & ~32'h00001888) |
                            (csr_trap_state_slv.pkt.mstatus[3] ? 32'h00000080 : 32'b0) |
                            {19'b0, exu_state_slv.pkt.priv, 11'b0});
                        wr_addr[1] <= delegated ? 12'h142 : 12'h342;
                        wr_val[1] <= {is_interrupt, 26'b0, cause};
                        wr_addr[2] <= delegated ? 12'h141 : 12'h341;
                        wr_val[2] <= epc & 32'hfffffffc;
                        wr_addr[3] <= delegated ? 12'h143 : 12'h343;
                        wr_val[3] <= ex_expt_slv.vld ? ex_expt_slv.pkt.tval : 0;
                        write_count <= 4;
                        target_pc <= (delegated ? {csr_trap_state_slv.pkt.stvec[31:2], 2'b00} :
                            {csr_trap_state_slv.pkt.mtvec[31:2], 2'b00}) +
                            ((is_interrupt && (delegated ? csr_trap_state_slv.pkt.stvec[1:0] == 2'b01 :
                            csr_trap_state_slv.pkt.mtvec[1:0] == 2'b01)) ?
                            {25'b0, cause, 2'b00} : 32'b0);
                        target_priv <= delegated ? 2'b01 : 2'b11;
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
    assign trap_exu_ctrl_mst.vld = (state == IDLE && event_valid) || state == REDIRECT;
    assign trap_exu_ctrl_mst.pkt.priv = state == REDIRECT ? target_priv : exu_state_slv.pkt.priv;
    assign trap_exu_ctrl_mst.pkt.irq_epc = target_pc;
    assign trap_exu_ctrl_mst.pkt.wfi = state != REDIRECT;
endmodule
