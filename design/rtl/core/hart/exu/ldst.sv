`include "spec/core/isa.svh"
`include "itf/ldst_req_if.svh"

module exu_ldst_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32g_inst_t inst,
    ldst_req_if_t.mst  ldst_req_mst,
    ldst_rsp_if_t.slv  ldst_rsp_slv,
    exu_gpr_if_t.mst   gpr_mst,
    output logic       done
);
    typedef enum logic [2:0] {
        IDLE,
        WAIT_NORMAL,
        WAIT_AMO_READ,
        SEND_AMO_WRITE,
        WAIT_AMO_WRITE
    } state_t;

    localparam logic [4:0] AMO_LR = 5'b00010;
    localparam logic [4:0] AMO_SC = 5'b00011;
    localparam logic [4:0] AMO_SWAP = 5'b00001;
    localparam logic [4:0] AMO_ADD = 5'b00000;
    localparam logic [4:0] AMO_XOR = 5'b00100;
    localparam logic [4:0] AMO_AND = 5'b01100;
    localparam logic [4:0] AMO_OR = 5'b01000;
    localparam logic [4:0] AMO_MIN = 5'b10000;
    localparam logic [4:0] AMO_MAX = 5'b10100;
    localparam logic [4:0] AMO_MINU = 5'b11000;
    localparam logic [4:0] AMO_MAXU = 5'b11100;

    state_t state;
    logic normal_load;
    logic [2:0] load_funct3;
    logic [4:0] req_rd;
    logic [4:0] amo_funct5;
    logic [31:0] amo_addr;
    logic [31:0] amo_src;
    logic [31:0] amo_old;
    logic [31:0] amo_new;
    logic reservation_valid;
    logic [31:0] reservation_addr;

    wire is_load = inst.base.opcode == OPCODE_LOAD;
    wire is_store = inst.base.opcode == OPCODE_STORE;
    wire is_amo = inst.base.opcode == OPCODE_AMO;
    wire [4:0] inst_amo_funct5 = inst.r.funct7[6:2];
    wire sc_fail = state == IDLE && sel && is_amo && inst_amo_funct5 == AMO_SC &&
        (!reservation_valid || reservation_addr != gpr_mst.rd1);
    wire req_hsk = ldst_req_mst.vld && ldst_req_mst.rdy;
    wire rsp_hsk = ldst_rsp_slv.vld && ldst_rsp_slv.rdy;

    logic [31:0] i_imm;
    logic [31:0] s_imm;
    i_imm_decode u_i_imm_decode(.inst(inst), .i_imm(i_imm));
    s_imm_decode u_s_imm_decode(.inst(inst), .s_imm(s_imm));

    always_comb begin
        case (amo_funct5)
            AMO_SWAP: amo_new = amo_src;
            AMO_ADD: amo_new = amo_old + amo_src;
            AMO_XOR: amo_new = amo_old ^ amo_src;
            AMO_AND: amo_new = amo_old & amo_src;
            AMO_OR: amo_new = amo_old | amo_src;
            AMO_MIN: amo_new = $signed(amo_old) < $signed(amo_src) ? amo_old : amo_src;
            AMO_MAX: amo_new = $signed(amo_old) > $signed(amo_src) ? amo_old : amo_src;
            AMO_MINU: amo_new = amo_old < amo_src ? amo_old : amo_src;
            AMO_MAXU: amo_new = amo_old > amo_src ? amo_old : amo_src;
            default: amo_new = amo_src;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            normal_load <= 1'b0;
            load_funct3 <= '0;
            req_rd <= '0;
            amo_funct5 <= '0;
            amo_addr <= '0;
            amo_src <= '0;
            amo_old <= '0;
            reservation_valid <= 1'b0;
            reservation_addr <= '0;
        end else begin
            case (state)
                IDLE: begin
                    if (sc_fail) begin
                        reservation_valid <= 1'b0;
                    end else if (req_hsk) begin
                        req_rd <= inst.r.rd;
                        if (is_amo) begin
                            amo_funct5 <= inst_amo_funct5;
                            amo_addr <= gpr_mst.rd1;
                            amo_src <= gpr_mst.rd2;
                            if (inst_amo_funct5 == AMO_SC) begin
                                reservation_valid <= 1'b0;
                                state <= WAIT_AMO_WRITE;
                            end else begin
                                state <= WAIT_AMO_READ;
                            end
                        end else begin
                            normal_load <= is_load;
                            load_funct3 <= inst.i.funct3;
                            if (is_store)
                                reservation_valid <= 1'b0;
                            state <= WAIT_NORMAL;
                        end
                    end
                end
                WAIT_NORMAL: if (rsp_hsk) state <= IDLE;
                WAIT_AMO_READ: begin
                    if (rsp_hsk) begin
                        amo_old <= ldst_rsp_slv.pkt.data;
                        if (amo_funct5 == AMO_LR) begin
                            reservation_valid <= 1'b1;
                            reservation_addr <= amo_addr;
                            state <= IDLE;
                        end else begin
                            state <= SEND_AMO_WRITE;
                        end
                    end
                end
                SEND_AMO_WRITE: if (req_hsk) state <= WAIT_AMO_WRITE;
                WAIT_AMO_WRITE: if (rsp_hsk) state <= IDLE;
                default: state <= IDLE;
            endcase
        end
    end

    always_comb begin
        gpr_mst.ra1 = '0;
        gpr_mst.ra2 = '0;
        if (state == IDLE && sel) begin
            if (is_load) begin
                gpr_mst.ra1 = inst.i.rs1;
            end else if (is_store || is_amo) begin
                gpr_mst.ra1 = inst.r.rs1;
                gpr_mst.ra2 = inst.r.rs2;
            end
        end
    end

    always_comb begin
        ldst_req_mst.vld = 1'b0;
        ldst_req_mst.pkt.addr = '0;
        ldst_req_mst.pkt.st = 1'b0;
        ldst_req_mst.pkt.size = LDST_REQ_SIZE_B4;
        ldst_req_mst.pkt.data = '0;
        ldst_req_mst.pkt.strobe = 4'b1111;
        if (state == IDLE && sel && !sc_fail) begin
            ldst_req_mst.vld = 1'b1;
            if (is_load) begin
                ldst_req_mst.pkt.addr = gpr_mst.rd1 + i_imm;
                ldst_req_mst.pkt.st = 1'b0;
                ldst_req_mst.pkt.size = ldst_req_size_t'(inst.i.funct3[1:0]);
            end else if (is_store) begin
                ldst_req_mst.pkt.addr = gpr_mst.rd1 + s_imm;
                ldst_req_mst.pkt.st = 1'b1;
                ldst_req_mst.pkt.size = ldst_req_size_t'(inst.s.funct3[1:0]);
                ldst_req_mst.pkt.data = gpr_mst.rd2;
                case (inst.s.funct3)
                    STORE_FUNCT3_SB: ldst_req_mst.pkt.strobe = 4'b0001;
                    STORE_FUNCT3_SH: ldst_req_mst.pkt.strobe = 4'b0011;
                    default: ldst_req_mst.pkt.strobe = 4'b1111;
                endcase
            end else if (is_amo) begin
                ldst_req_mst.pkt.addr = gpr_mst.rd1;
                ldst_req_mst.pkt.st = inst_amo_funct5 == AMO_SC;
                ldst_req_mst.pkt.data = gpr_mst.rd2;
            end
        end else if (state == SEND_AMO_WRITE) begin
            ldst_req_mst.vld = 1'b1;
            ldst_req_mst.pkt.addr = amo_addr;
            ldst_req_mst.pkt.st = 1'b1;
            ldst_req_mst.pkt.data = amo_new;
        end
    end

    assign ldst_rsp_slv.rdy = 1'b1;
    assign done = sc_fail ||
        (rsp_hsk && (state == WAIT_NORMAL ||
        (state == WAIT_AMO_READ && amo_funct5 == AMO_LR) || state == WAIT_AMO_WRITE));

    always_comb begin
        gpr_mst.wen = 1'b0;
        gpr_mst.wa = req_rd;
        gpr_mst.wd = ldst_rsp_slv.pkt.data;
        if (sc_fail) begin
            gpr_mst.wen = 1'b1;
            gpr_mst.wa = inst.r.rd;
            gpr_mst.wd = 32'd1;
        end else if (rsp_hsk && state == WAIT_NORMAL && normal_load) begin
            gpr_mst.wen = 1'b1;
            case (load_funct3)
                LOAD_FUNCT3_LB: gpr_mst.wd = {{24{ldst_rsp_slv.pkt.data[7]}}, ldst_rsp_slv.pkt.data[7:0]};
                LOAD_FUNCT3_LH: gpr_mst.wd = {{16{ldst_rsp_slv.pkt.data[15]}}, ldst_rsp_slv.pkt.data[15:0]};
                LOAD_FUNCT3_LBU: gpr_mst.wd = {24'b0, ldst_rsp_slv.pkt.data[7:0]};
                LOAD_FUNCT3_LHU: gpr_mst.wd = {16'b0, ldst_rsp_slv.pkt.data[15:0]};
                default: gpr_mst.wd = ldst_rsp_slv.pkt.data;
            endcase
        end else if (rsp_hsk && state == WAIT_AMO_READ && amo_funct5 == AMO_LR) begin
            gpr_mst.wen = 1'b1;
            gpr_mst.wd = ldst_rsp_slv.pkt.data;
        end else if (rsp_hsk && state == WAIT_AMO_WRITE) begin
            gpr_mst.wen = 1'b1;
            gpr_mst.wd = amo_funct5 == AMO_SC ? 32'd0 : amo_old;
        end
    end
endmodule
