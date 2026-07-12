`include "spec/core/isa.svh"
`include "itf/hart_expt_if.svh"

module exu_sys_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32g_inst_t inst,
    exu_gpr_if_t.mst   gpr_mst,
    exu_csr_read_req_if_t.mst csr_read_req_mst,
    csr_exu_read_rsp_if_t.slv csr_read_rsp_slv,
    exu_csr_write_req_if_t.mst csr_write_req_mst,
    csr_exu_write_rsp_if_t.slv csr_write_rsp_slv,
    tlb_flush_if_t.mst tlb_flush_mst,
    l1_flush_if_t.mst l1i_flush_mst,
    l1_flush_if_t.mst l1d_flush_mst,
    input logic [1:0] priv,
    input logic [31:0] pc,
    output logic done,
    hart_expt_if_t.mst ex_expt_mst
);
    wire csr_inst = sel && inst.base.opcode == OPCODE_SYSTEM && inst.i.funct3 != 3'b000;
    wire csr_imm = inst.i.funct3[2];
    wire [31:0] csr_src = csr_imm ? {27'b0, inst.i.rs1} : gpr_mst.rd1;
    logic [31:0] csr_write_val;
    logic csr_write;
    wire sfence_vma = sel && inst.base.opcode == OPCODE_SYSTEM &&
        inst.i.funct3 == 3'b000 && inst.r.funct7 == 7'b0001001 &&
        inst.r.rd == 5'b00000;
    wire fence_i = sel && inst.base.opcode == OPCODE_MISC_MEM &&
        inst.i.funct3 == 3'b001;

    always_comb begin
        csr_write_val = csr_read_rsp_slv.pkt.val;
        csr_write = 1'b0;
        case (inst.i.funct3)
            3'b001, 3'b101: begin
                csr_write_val = csr_src;
                csr_write = 1'b1;
            end
            3'b010, 3'b110: begin
                csr_write_val = csr_read_rsp_slv.pkt.val | csr_src;
                csr_write = csr_src != 0;
            end
            3'b011, 3'b111: begin
                csr_write_val = csr_read_rsp_slv.pkt.val & ~csr_src;
                csr_write = csr_src != 0;
            end
            default: ;
        endcase
    end

    assign gpr_mst.ra1 = csr_inst && !csr_imm ? inst.i.rs1 : {`RV_GPR_AW{1'b0}};
    assign gpr_mst.ra2 = {`RV_GPR_AW{1'b0}};
    assign gpr_mst.wen = csr_inst && csr_read_rsp_slv.pkt.ok && inst.i.rd != 0;
    assign gpr_mst.wa = inst.i.rd;
    assign gpr_mst.wd = csr_read_rsp_slv.pkt.val;

    assign csr_read_req_mst.pkt.addr = inst.i.imm_11_0;
    assign csr_read_req_mst.pkt.priv = priv;
    assign csr_write_req_mst.vld = csr_inst && csr_write;
    assign csr_write_req_mst.pkt.addr = inst.i.imm_11_0;
    assign csr_write_req_mst.pkt.val = csr_write_val;
    assign csr_write_req_mst.pkt.priv = priv;
    assign tlb_flush_mst.vld = sfence_vma;
    assign l1i_flush_mst.vld = fence_i;
    assign l1d_flush_mst.vld = fence_i;
    assign done = 1'b1;

    assign ex_expt_mst.vld = sel && inst.base.opcode == OPCODE_SYSTEM &&
        inst.i.funct3 == 0 && (inst.i.imm_11_0 == 12'h000 ||
        inst.i.imm_11_0 == 12'h302 || inst.i.imm_11_0 == 12'h102);
    assign ex_expt_mst.pkt.expt_type = inst.i.imm_11_0 == 12'h302 ? HART_EXPT_TYPE_MRET :
        (inst.i.imm_11_0 == 12'h102 ? HART_EXPT_TYPE_SRET : HART_EXPT_TYPE_EXCEPTION);
    assign ex_expt_mst.pkt.cause = HART_EXPT_CAUSE_ECALL;
    assign ex_expt_mst.pkt.priv = priv;
    assign ex_expt_mst.pkt.pc = pc;
    assign ex_expt_mst.pkt.tval = 32'b0;
endmodule
