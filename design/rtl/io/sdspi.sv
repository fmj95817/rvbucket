`include "spec/io/sdspi.svh"
`include "itf/sdspi_cmd_if.svh"
`include "itf/sdspi_data_if.svh"

module sdspi #(
    parameter logic [31:0] BASE = 32'h30100000
)(
    input logic clk,
    input logic rst_n,
    apb_req_if_t.slv apb_req_slv,
    apb_rsp_if_t.mst apb_rsp_mst,
    axi4_aw_if_t.mst dma_axi4_aw_mst,
    axi4_w_if_t.mst  dma_axi4_w_mst,
    axi4_b_if_t.slv  dma_axi4_b_slv,
    axi4_ar_if_t.mst dma_axi4_ar_mst,
    axi4_r_if_t.slv  dma_axi4_r_slv,
    ext_irq_if_t.mst irq_mst,
    sdspi_cmd_if_t.mst cmd_mst,
    sdspi_data_if_t.slv data_slv
);
    localparam int FIFO_WORDS = 64;
    localparam logic [7:0] DMA_ID = 8'hff;

    typedef enum logic [2:0] {
        STATE_IDLE,
        STATE_WRITE_PREFETCH,
        STATE_SEND_CMD,
        STATE_WAIT_CMD_RSP,
        STATE_READ_STREAM,
        STATE_WRITE_STREAM,
        STATE_WAIT_WRITE_DONE
    } state_t;

    typedef enum logic [2:0] {
        DMA_IDLE,
        DMA_TO_MEM_AW,
        DMA_TO_MEM_W,
        DMA_TO_MEM_B,
        DMA_TO_MEM_SYNC_AR,
        DMA_TO_MEM_SYNC_R,
        DMA_FROM_MEM_AR,
        DMA_FROM_MEM_R
    } dma_state_t;

    logic [31:0] ctrl;
    logic [31:0] clock_div;
    logic [31:0] cmd_arg;
    logic [31:0] cmd_ctrl;
    logic [31:0] cmd_status;
    logic [31:0] resp0;
    logic [31:0] resp1;
    logic [31:0] dma_addr;
    logic [31:0] block_size;
    logic [31:0] block_count;
    logic [31:0] data_status;
    logic [31:0] irq_status;
    logic [31:0] irq_enable;

    logic card_present;
    logic read_only;
    logic card_idle;
    logic card_status_valid;
    logic reset_pending;
    logic config_pending;
    logic data_write;
    logic read_input_done;
    logic [31:0] data_len;
    logic [31:0] protocol_offset;
    state_t state;
    dma_state_t dma_state;
    logic [31:0] dma_offset;
    logic [31:0] dma_sync_offset;
    logic [31:0] burst_bytes;
    logic [4:0] burst_beat;
    logic [4:0] burst_beats;

    logic [31:0] stream_fifo[0:FIFO_WORDS-1];
    logic [5:0] fifo_rptr;
    logic [5:0] fifo_wptr;
    logic [6:0] fifo_count;
    logic fifo_push;
    logic [31:0] fifo_push_data;
    logic fifo_pop;

    logic apb_rsp_pending;
    logic [31:0] apb_rsp_data;
    logic apb_rsp_error;
    logic [31:0] apb_read_data;
    logic apb_addr_valid;
    logic apb_reg_valid;
    logic apb_write_ok;
    logic [5:0] apb_offset;
    logic [31:0] apb_merged;

    wire apb_req_hsk = apb_req_slv.psel && apb_req_slv.penable &&
        !apb_rsp_pending;
    wire cmd_hsk = cmd_mst.vld && cmd_mst.rdy;
    wire data_hsk = data_slv.vld && data_slv.rdy;
    wire dma_aw_hsk = dma_axi4_aw_mst.vld && dma_axi4_aw_mst.rdy;
    wire dma_w_hsk = dma_axi4_w_mst.vld && dma_axi4_w_mst.rdy;
    wire dma_b_hsk = dma_axi4_b_slv.vld && dma_axi4_b_slv.rdy;
    wire dma_ar_hsk = dma_axi4_ar_mst.vld && dma_axi4_ar_mst.rdy;
    wire dma_r_hsk = dma_axi4_r_slv.vld && dma_axi4_r_slv.rdy;
    wire busy = state != STATE_IDLE || dma_state != DMA_IDLE || reset_pending;

    function automatic logic [31:0] merge_write(
        input logic [31:0] old_value,
        input logic [31:0] new_value,
        input logic [3:0] strobe
    );
        logic [31:0] value;
        value = old_value;
        for (int i = 0; i < 4; i++) begin
            if (strobe[i]) value[i * 8 +: 8] = new_value[i * 8 +: 8];
        end
        return value;
    endfunction

    function automatic logic [4:0] next_burst_beats(
        input logic [31:0] offset,
        input logic [6:0] max_beats
    );
        logic [31:0] addr;
        logic [31:0] bytes;
        logic [31:0] boundary;
        logic [31:0] max_bytes;
        addr = dma_addr + offset;
        bytes = data_len - offset;
        boundary = 32'h1000 - {20'b0, addr[11:0]};
        max_bytes = {25'b0, max_beats} << 2;
        if (bytes > boundary) bytes = boundary;
        if (bytes > max_bytes) bytes = max_bytes;
        return bytes[6:2];
    endfunction

    task automatic clear_fifo;
        begin
            fifo_rptr <= '0;
            fifo_wptr <= '0;
            fifo_count <= '0;
        end
    endtask

    task automatic reset_registers;
        begin
            ctrl <= '0;
            clock_div <= '0;
            cmd_arg <= '0;
            cmd_ctrl <= '0;
            cmd_status <= '0;
            resp0 <= '0;
            resp1 <= '0;
            dma_addr <= '0;
            block_size <= `SDSPI_SECTOR_SIZE;
            block_count <= 32'd1;
            data_status <= '0;
            irq_status <= '0;
            irq_enable <= '0;
            config_pending <= 1'b1;
            data_write <= 1'b0;
            read_input_done <= 1'b0;
            data_len <= '0;
            protocol_offset <= '0;
            state <= STATE_IDLE;
            dma_state <= DMA_IDLE;
            dma_offset <= '0;
            dma_sync_offset <= '0;
            burst_bytes <= '0;
            burst_beat <= '0;
            burst_beats <= '0;
            clear_fifo();
        end
    endtask

    task automatic finish_command(
        input logic has_data,
        input logic error,
        input logic [31:0] cmd_error,
        input logic [31:0] data_error
    );
        logic [31:0] cmd_value;
        logic [31:0] data_value;
        logic [31:0] irq_value;
        begin
            cmd_value = `SDSPI_CMD_STATUS_DONE | cmd_error;
            data_value = has_data ? (`SDSPI_DATA_STATUS_DONE | data_error) : 0;
            irq_value = irq_status | `SDSPI_IRQ_CMD_DONE;
            if (has_data) irq_value = irq_value | `SDSPI_IRQ_DATA_DONE;
            if (error) begin
                cmd_value = cmd_value | `SDSPI_CMD_STATUS_ERROR;
                if (has_data)
                    data_value = data_value | `SDSPI_DATA_STATUS_ERROR;
                irq_value = irq_value | `SDSPI_IRQ_ERROR;
            end
            state <= STATE_IDLE;
            dma_state <= DMA_IDLE;
            cmd_status <= cmd_value;
            data_status <= data_value;
            irq_status <= irq_value;
            clear_fifo();
        end
    endtask

    always_comb begin
        apb_addr_valid = apb_req_slv.pkt.paddr >= BASE &&
            apb_req_slv.pkt.paddr < BASE + 32'h1000 &&
            apb_req_slv.pkt.paddr[1:0] == 2'b00;
        apb_offset = apb_req_slv.pkt.paddr[5:0];
        apb_reg_valid = 1'b1;
        apb_read_data = '0;
        unique case (apb_offset)
            `SDSPI_REG_VERSION: apb_read_data = `SDSPI_VERSION;
            `SDSPI_REG_CAP: apb_read_data = `SDSPI_CAP;
            `SDSPI_REG_CTRL: apb_read_data = ctrl;
            `SDSPI_REG_CLOCK: apb_read_data = clock_div;
            `SDSPI_REG_STATUS: begin
                if (card_present) apb_read_data |= `SDSPI_STATUS_CARD_PRESENT;
                if (read_only) apb_read_data |= `SDSPI_STATUS_CARD_RO;
                if (busy) apb_read_data |= `SDSPI_STATUS_BUSY;
                if (card_idle) apb_read_data |= `SDSPI_STATUS_CARD_IDLE;
            end
            `SDSPI_REG_CMD_ARG: apb_read_data = cmd_arg;
            `SDSPI_REG_CMD_CTRL: apb_read_data = cmd_ctrl;
            `SDSPI_REG_CMD_STATUS: apb_read_data = cmd_status;
            `SDSPI_REG_RESP0: apb_read_data = resp0;
            `SDSPI_REG_RESP1: apb_read_data = resp1;
            `SDSPI_REG_DMA_ADDR: apb_read_data = dma_addr;
            `SDSPI_REG_BLOCK_SIZE: apb_read_data = block_size;
            `SDSPI_REG_BLOCK_COUNT: apb_read_data = block_count;
            `SDSPI_REG_DATA_STATUS: apb_read_data = data_status;
            `SDSPI_REG_IRQ_STATUS: apb_read_data = irq_status;
            `SDSPI_REG_IRQ_ENABLE: apb_read_data = irq_enable;
            default: apb_reg_valid = 1'b0;
        endcase

        apb_write_ok = apb_addr_valid && apb_reg_valid;
        apb_merged = apb_req_slv.pkt.pwdata;
        if (apb_req_slv.pkt.pwrite) begin
            unique case (apb_offset)
                `SDSPI_REG_CTRL: begin
                    apb_merged = merge_write(ctrl, apb_req_slv.pkt.pwdata,
                        apb_req_slv.pkt.pstrb);
                    if (!(| (apb_merged & `SDSPI_CTRL_SW_RESET)) && busy)
                        apb_write_ok = 1'b0;
                end
                `SDSPI_REG_CLOCK: begin
                    apb_merged = merge_write(clock_div,
                        apb_req_slv.pkt.pwdata, apb_req_slv.pkt.pstrb);
                    if (busy) apb_write_ok = 1'b0;
                end
                `SDSPI_REG_CMD_ARG:
                    apb_merged = merge_write(cmd_arg,
                        apb_req_slv.pkt.pwdata, apb_req_slv.pkt.pstrb);
                `SDSPI_REG_CMD_CTRL: begin
                    apb_merged = merge_write(cmd_ctrl,
                        apb_req_slv.pkt.pwdata, apb_req_slv.pkt.pstrb);
                    if ((apb_merged & `SDSPI_CMD_CTRL_START) != 0 && busy)
                        apb_write_ok = 1'b0;
                end
                `SDSPI_REG_DMA_ADDR:
                    apb_merged = merge_write(dma_addr,
                        apb_req_slv.pkt.pwdata, apb_req_slv.pkt.pstrb);
                `SDSPI_REG_BLOCK_SIZE:
                    apb_merged = merge_write(block_size,
                        apb_req_slv.pkt.pwdata, apb_req_slv.pkt.pstrb);
                `SDSPI_REG_BLOCK_COUNT:
                    apb_merged = merge_write(block_count,
                        apb_req_slv.pkt.pwdata, apb_req_slv.pkt.pstrb);
                `SDSPI_REG_IRQ_STATUS: apb_merged = apb_req_slv.pkt.pwdata;
                `SDSPI_REG_IRQ_ENABLE:
                    apb_merged = merge_write(irq_enable,
                        apb_req_slv.pkt.pwdata, apb_req_slv.pkt.pstrb);
                default: apb_write_ok = 1'b0;
            endcase
        end
    end

    always_comb begin
        cmd_mst.vld = 1'b0;
        cmd_mst.pkt = '0;
        if (reset_pending) begin
            cmd_mst.vld = 1'b1;
            cmd_mst.pkt.kind = SDSPI_CMD_KIND_RESET;
        end else if (config_pending) begin
            cmd_mst.vld = 1'b1;
            cmd_mst.pkt.kind = SDSPI_CMD_KIND_CONFIG;
            cmd_mst.pkt.enable = ctrl[0];
            cmd_mst.pkt.clock_div = clock_div;
        end else if (state == STATE_SEND_CMD) begin
            cmd_mst.vld = 1'b1;
            cmd_mst.pkt.kind = SDSPI_CMD_KIND_COMMAND;
            cmd_mst.pkt.opcode = cmd_ctrl[5:0];
            cmd_mst.pkt.arg = cmd_arg;
            cmd_mst.pkt.rsp_type = cmd_ctrl[10:8];
            cmd_mst.pkt.data_present = data_len != 0;
            cmd_mst.pkt.write = data_write;
            cmd_mst.pkt.block_size = block_size;
            cmd_mst.pkt.block_count = block_count;
            cmd_mst.pkt.data_len = data_len;
        end else if (state == STATE_WRITE_STREAM && fifo_count != 0) begin
            cmd_mst.vld = 1'b1;
            cmd_mst.pkt.kind = SDSPI_CMD_KIND_WRITE_DATA;
            cmd_mst.pkt.data = stream_fifo[fifo_rptr];
            cmd_mst.pkt.last = protocol_offset + 4 == data_len;
        end
    end

    always_comb begin
        data_slv.rdy = 1'b1;
        if (data_slv.pkt.kind == SDSPI_DATA_KIND_READ_DATA)
            data_slv.rdy = state == STATE_READ_STREAM &&
                fifo_count != 7'd64;
    end

    always_comb begin
        dma_axi4_aw_mst.vld = dma_state == DMA_TO_MEM_AW;
        dma_axi4_aw_mst.pkt = '0;
        dma_axi4_aw_mst.pkt.id = DMA_ID;
        dma_axi4_aw_mst.pkt.addr = dma_addr + dma_offset;
        dma_axi4_aw_mst.pkt.len = {3'b0, burst_beats} - 8'd1;
        dma_axi4_aw_mst.pkt.size = AXI4_AW_SIZE_B4;
        dma_axi4_aw_mst.pkt.burst = AXI4_AW_BURST_INCR;

        dma_axi4_w_mst.vld = dma_state == DMA_TO_MEM_W && fifo_count != 0;
        dma_axi4_w_mst.pkt = '0;
        dma_axi4_w_mst.pkt.data = stream_fifo[fifo_rptr];
        dma_axi4_w_mst.pkt.strb = 4'hf;
        dma_axi4_w_mst.pkt.last = burst_beat + 1'b1 == burst_beats;
        dma_axi4_b_slv.rdy = dma_state == DMA_TO_MEM_B;

        dma_axi4_ar_mst.vld = dma_state == DMA_FROM_MEM_AR ||
            dma_state == DMA_TO_MEM_SYNC_AR;
        dma_axi4_ar_mst.pkt = '0;
        dma_axi4_ar_mst.pkt.id = DMA_ID;
        if (dma_state == DMA_TO_MEM_SYNC_AR) begin
            // Drain the completed DMA range before signaling software.
            dma_axi4_ar_mst.pkt.addr = dma_addr + dma_sync_offset;
            dma_axi4_ar_mst.pkt.len = {3'b0, burst_beats} - 8'd1;
        end else begin
            dma_axi4_ar_mst.pkt.addr = dma_addr + dma_offset;
            dma_axi4_ar_mst.pkt.len = {3'b0, burst_beats} - 8'd1;
        end
        dma_axi4_ar_mst.pkt.size = AXI4_AR_SIZE_B4;
        dma_axi4_ar_mst.pkt.burst = AXI4_AR_BURST_INCR;
        dma_axi4_r_slv.rdy = (dma_state == DMA_FROM_MEM_R &&
            fifo_count != 7'd64) || dma_state == DMA_TO_MEM_SYNC_R;
    end

    always_comb begin
        fifo_push = 1'b0;
        fifo_push_data = '0;
        if (data_hsk && data_slv.pkt.kind == SDSPI_DATA_KIND_READ_DATA) begin
            fifo_push = 1'b1;
            fifo_push_data = data_slv.pkt.data;
        end else if (dma_r_hsk && dma_state == DMA_FROM_MEM_R) begin
            fifo_push = 1'b1;
            fifo_push_data = dma_axi4_r_slv.pkt.data;
        end
        fifo_pop = dma_w_hsk || (cmd_hsk &&
            cmd_mst.pkt.kind == SDSPI_CMD_KIND_WRITE_DATA);
    end

    assign apb_rsp_mst.pready = apb_rsp_pending;
    assign apb_rsp_mst.pkt.prdata = apb_rsp_data;
    assign apb_rsp_mst.pkt.pslverr = apb_rsp_error;
    assign irq_mst.pkt.irq = |(irq_status & irq_enable);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            card_present <= 1'b0;
            read_only <= 1'b0;
            card_idle <= 1'b1;
            card_status_valid <= 1'b0;
            reset_pending <= 1'b0;
            apb_rsp_pending <= 1'b0;
            apb_rsp_data <= '0;
            apb_rsp_error <= 1'b0;
            reset_registers();
        end else begin
            if (apb_rsp_pending) apb_rsp_pending <= 1'b0;

            unique case ({fifo_push, fifo_pop})
                2'b10: fifo_count <= fifo_count + 1'b1;
                2'b01: fifo_count <= fifo_count - 1'b1;
                default: fifo_count <= fifo_count;
            endcase
            if (fifo_push) begin
                stream_fifo[fifo_wptr] <= fifo_push_data;
                fifo_wptr <= fifo_wptr + 1'b1;
            end
            if (fifo_pop) fifo_rptr <= fifo_rptr + 1'b1;

            if (cmd_hsk) begin
                unique case (cmd_mst.pkt.kind)
                    SDSPI_CMD_KIND_RESET: reset_pending <= 1'b0;
                    SDSPI_CMD_KIND_CONFIG: config_pending <= 1'b0;
                    SDSPI_CMD_KIND_COMMAND: state <= STATE_WAIT_CMD_RSP;
                    SDSPI_CMD_KIND_WRITE_DATA: begin
                        protocol_offset <= protocol_offset + 4;
                        if (cmd_mst.pkt.last) state <= STATE_WAIT_WRITE_DONE;
                    end
                endcase
            end

            if (data_hsk) begin
                if (card_status_valid &&
                    (card_present != data_slv.pkt.card_present ||
                     read_only != data_slv.pkt.read_only))
                    irq_status <= irq_status | `SDSPI_IRQ_CARD_CHANGE;
                card_present <= data_slv.pkt.card_present;
                read_only <= data_slv.pkt.read_only;
                card_idle <= data_slv.pkt.card_idle;
                card_status_valid <= 1'b1;

                unique case (data_slv.pkt.kind)
                    SDSPI_DATA_KIND_STATUS: begin end
                    SDSPI_DATA_KIND_RESPONSE: begin
                        if (state != STATE_WAIT_CMD_RSP) begin
                            if (state != STATE_IDLE) begin
                                reset_pending <= 1'b1;
                                finish_command(data_len != 0, 1'b1, 0,
                                    `SDSPI_DATA_STATUS_IO_ERROR);
                            end
                        end else begin
                            resp0 <= data_slv.pkt.resp0;
                            resp1 <= data_slv.pkt.resp1;
                            if (data_slv.pkt.error)
                                finish_command(data_len != 0, 1'b1,
                                    data_slv.pkt.cmd_error,
                                    data_slv.pkt.data_error);
                            else if (data_len == 0)
                                finish_command(1'b0, 1'b0, 0, 0);
                            else if (data_write)
                                state <= STATE_WRITE_STREAM;
                            else
                                state <= STATE_READ_STREAM;
                        end
                    end
                    SDSPI_DATA_KIND_READ_DATA: begin
                        if (state != STATE_READ_STREAM ||
                            protocol_offset + 4 > data_len ||
                            data_slv.pkt.last !=
                                (protocol_offset + 4 == data_len)) begin
                            reset_pending <= 1'b1;
                            finish_command(1'b1, 1'b1, 0,
                                `SDSPI_DATA_STATUS_IO_ERROR);
                        end else begin
                            protocol_offset <= protocol_offset + 4;
                            if (data_slv.pkt.last) read_input_done <= 1'b1;
                        end
                    end
                    SDSPI_DATA_KIND_WRITE_DONE: begin
                        if (state != STATE_WAIT_WRITE_DONE ||
                            protocol_offset != data_len ||
                            dma_offset != data_len ||
                            dma_state != DMA_IDLE || fifo_count != 0) begin
                            reset_pending <= 1'b1;
                            finish_command(1'b1, 1'b1, 0,
                                `SDSPI_DATA_STATUS_IO_ERROR);
                        end else begin
                            finish_command(1'b1, data_slv.pkt.error,
                                data_slv.pkt.cmd_error,
                                data_slv.pkt.data_error);
                        end
                    end
                endcase
            end

            if (dma_aw_hsk) dma_state <= DMA_TO_MEM_W;
            if (dma_w_hsk) begin
                burst_beat <= burst_beat + 1'b1;
                if (dma_axi4_w_mst.pkt.last) dma_state <= DMA_TO_MEM_B;
            end
            if (dma_b_hsk) begin
                if (dma_axi4_b_slv.pkt.id != DMA_ID ||
                    dma_axi4_b_slv.pkt.resp != AXI4_B_RESP_OKAY) begin
                    reset_pending <= 1'b1;
                    finish_command(1'b1, 1'b1, 0,
                        `SDSPI_DATA_STATUS_AXI_ERROR);
                end else begin
                    dma_offset <= dma_offset + burst_bytes;
                    if (dma_offset + burst_bytes == data_len) begin
                        logic [4:0] beats;
                        beats = next_burst_beats(0, 7'd16);
                        dma_sync_offset <= 0;
                        burst_beats <= beats;
                        burst_bytes <= {25'b0, beats, 2'b0};
                        burst_beat <= 0;
                        dma_state <= DMA_TO_MEM_SYNC_AR;
                    end else begin
                        dma_state <= DMA_IDLE;
                    end
                end
            end

            if (dma_ar_hsk) begin
                if (dma_state == DMA_TO_MEM_SYNC_AR)
                    dma_state <= DMA_TO_MEM_SYNC_R;
                else
                    dma_state <= DMA_FROM_MEM_R;
            end
            if (dma_r_hsk) begin
                if (dma_state == DMA_TO_MEM_SYNC_R) begin
                    logic expected_last;
                    expected_last = burst_beat + 1'b1 == burst_beats;
                    if (dma_axi4_r_slv.pkt.id != DMA_ID ||
                        dma_axi4_r_slv.pkt.resp != AXI4_R_RESP_OKAY ||
                        dma_axi4_r_slv.pkt.last != expected_last) begin
                        reset_pending <= 1'b1;
                        finish_command(1'b1, 1'b1, 0,
                            `SDSPI_DATA_STATUS_AXI_ERROR);
                    end else begin
                        burst_beat <= burst_beat + 1'b1;
                        if (dma_axi4_r_slv.pkt.last) begin
                            logic [31:0] next_offset;
                            next_offset = dma_sync_offset + burst_bytes;
                            dma_sync_offset <= next_offset;
                            if (next_offset == data_len) begin
                                if (!read_input_done) begin
                                    reset_pending <= 1'b1;
                                    finish_command(1'b1, 1'b1, 0,
                                        `SDSPI_DATA_STATUS_IO_ERROR);
                                end else begin
                                    finish_command(1'b1, 1'b0, 0, 0);
                                end
                            end else begin
                                logic [4:0] beats;
                                beats = next_burst_beats(next_offset, 7'd16);
                                burst_beats <= beats;
                                burst_bytes <= {25'b0, beats, 2'b0};
                                burst_beat <= 0;
                                dma_state <= DMA_TO_MEM_SYNC_AR;
                            end
                        end
                    end
                end else if (dma_axi4_r_slv.pkt.id != DMA_ID ||
                    dma_axi4_r_slv.pkt.resp != AXI4_R_RESP_OKAY ||
                    dma_axi4_r_slv.pkt.last !=
                        (burst_beat + 1'b1 == burst_beats)) begin
                    reset_pending <= 1'b1;
                    finish_command(1'b1, 1'b1, 0,
                        `SDSPI_DATA_STATUS_AXI_ERROR);
                end else begin
                    burst_beat <= burst_beat + 1'b1;
                    if (dma_axi4_r_slv.pkt.last) begin
                        dma_offset <= dma_offset + burst_bytes;
                        dma_state <= DMA_IDLE;
                    end
                end
            end

            if (state == STATE_WRITE_PREFETCH) begin
                if ({25'b0, fifo_count} >=
                        ((data_len >> 2) > 32'd16 ?
                            32'd16 : (data_len >> 2)) ||
                    (dma_offset == data_len && dma_state == DMA_IDLE))
                    state <= STATE_SEND_CMD;
            end

            if (dma_state == DMA_IDLE && dma_offset != data_len) begin
                logic [6:0] max_beats;
                logic [4:0] beats;
                if (state == STATE_WRITE_PREFETCH ||
                    state == STATE_WRITE_STREAM) begin
                    max_beats = 7'd64 - fifo_count;
                    if (max_beats > 7'd16) max_beats = 7'd16;
                    if (max_beats != 0) begin
                        beats = next_burst_beats(dma_offset, max_beats);
                        burst_beats <= beats;
                        burst_bytes <= {25'b0, beats, 2'b0};
                        burst_beat <= '0;
                        dma_state <= DMA_FROM_MEM_AR;
                    end
                end else if (state == STATE_READ_STREAM && fifo_count != 0) begin
                    beats = next_burst_beats(dma_offset, 7'd16);
                    burst_beats <= beats;
                    burst_bytes <= {25'b0, beats, 2'b0};
                    burst_beat <= '0;
                    dma_state <= DMA_TO_MEM_AW;
                end
            end

            if (apb_req_hsk) begin
                apb_rsp_pending <= 1'b1;
                apb_rsp_data <= apb_read_data;
                apb_rsp_error <= !apb_addr_valid || !apb_reg_valid ||
                    (apb_req_slv.pkt.pwrite && !apb_write_ok);
                if (apb_req_slv.pkt.pwrite && apb_write_ok) begin
                    unique case (apb_offset)
                        `SDSPI_REG_CTRL: begin
                            if ((apb_merged & `SDSPI_CTRL_SW_RESET) != 0) begin
                                reset_registers();
                                reset_pending <= 1'b1;
                            end else begin
                                if (ctrl != (apb_merged & `SDSPI_CTRL_ENABLE))
                                    config_pending <= 1'b1;
                                ctrl <= apb_merged & `SDSPI_CTRL_ENABLE;
                            end
                        end
                        `SDSPI_REG_CLOCK: begin
                            if (clock_div != apb_merged) config_pending <= 1'b1;
                            clock_div <= apb_merged;
                        end
                        `SDSPI_REG_CMD_ARG: cmd_arg <= apb_merged;
                        `SDSPI_REG_CMD_CTRL: begin
                            if ((apb_merged & `SDSPI_CMD_CTRL_START) == 0) begin
                                cmd_ctrl <= apb_merged;
                            end else begin
                                logic has_data;
                                logic write_data;
                                logic [63:0] length;
                                has_data = |(apb_merged & `SDSPI_CMD_CTRL_DATA);
                                write_data = |(apb_merged &
                                    `SDSPI_CMD_CTRL_WRITE);
                                length = block_size * block_count;
                                cmd_ctrl <= apb_merged &
                                    ~`SDSPI_CMD_CTRL_START;
                                cmd_status <= 0;
                                data_status <= 0;
                                resp0 <= 0;
                                resp1 <= 0;
                                data_write <= write_data;
                                read_input_done <= 1'b0;
                                protocol_offset <= 0;
                                dma_offset <= 0;
                                dma_sync_offset <= 0;
                                dma_state <= DMA_IDLE;
                                clear_fifo();
                                if ((ctrl & `SDSPI_CTRL_ENABLE) == 0 ||
                                    !card_present) begin
                                    finish_command(has_data, 1'b1,
                                        `SDSPI_CMD_STATUS_TIMEOUT, 0);
                                end else if (write_data && !has_data) begin
                                    finish_command(1'b0, 1'b1,
                                        `SDSPI_CMD_STATUS_PARAMETER, 0);
                                end else if (has_data &&
                                    (length == 0 ||
                                     length > 64'd65536 ||
                                     dma_addr[1:0] != 0 || length[1:0] != 0)) begin
                                    finish_command(1'b1, 1'b1,
                                        `SDSPI_CMD_STATUS_PARAMETER, 0);
                                end else begin
                                    data_len <= has_data ? length[31:0] : 0;
                                    state <= write_data ? STATE_WRITE_PREFETCH :
                                        STATE_SEND_CMD;
                                end
                            end
                        end
                        `SDSPI_REG_DMA_ADDR: dma_addr <= apb_merged;
                        `SDSPI_REG_BLOCK_SIZE: block_size <= apb_merged;
                        `SDSPI_REG_BLOCK_COUNT: block_count <= apb_merged;
                        `SDSPI_REG_IRQ_STATUS:
                            irq_status <= irq_status &
                                ~(apb_merged & `SDSPI_IRQ_MASK);
                        `SDSPI_REG_IRQ_ENABLE:
                            irq_enable <= apb_merged & `SDSPI_IRQ_MASK;
                        default: begin end
                    endcase
                end
            end
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n) begin
            assert (fifo_count <= 7'd64)
                else $fatal(1, "sdspi stream FIFO overflow");
            if (dma_state != DMA_IDLE)
                assert (burst_beats != 0)
                    else $fatal(1, "sdspi zero-length DMA burst");
        end
    end
`endif
endmodule
