`include "spec/io/sdspi.svh"
`include "itf/sdspi_cmd_if.svh"
`include "itf/sdspi_data_if.svh"

module sdspi_phy #(
    parameter int RESPONSE_TIMEOUT_BYTES = 1024,
    // Allow 2.56 s of card busy time at the full-system 25 MHz SPI clock.
    parameter int DATA_TIMEOUT_BYTES = 8000000
)(
    input logic clk,
    input logic rst_n,
    sdspi_cmd_if_t.slv cmd_slv,
    sdspi_data_if_t.mst data_mst,
    output logic spi_clk,
    output logic spi_cs_n,
    output logic spi_mosi,
    input logic spi_miso
);
    typedef enum logic [4:0] {
        PHY_IDLE,
        PHY_INIT_CLOCKS,
        PHY_CMD_SEND,
        PHY_CMD_SKIP_STUFF,
        PHY_CMD_WAIT_R1,
        PHY_CMD_READ_EXTRA,
        PHY_CMD_WAIT_BUSY,
        PHY_SEND_RESPONSE,
        PHY_READ_WAIT_TOKEN,
        PHY_READ_BYTES,
        PHY_READ_SEND_WORD,
        PHY_READ_CRC,
        PHY_WRITE_TOKEN,
        PHY_WRITE_WAIT_WORD,
        PHY_WRITE_BYTES,
        PHY_WRITE_CRC,
        PHY_WRITE_RESPONSE,
        PHY_WRITE_BUSY,
        PHY_WRITE_STOP,
        PHY_WRITE_STOP_BUSY,
        PHY_SEND_WRITE_DONE,
        PHY_END_CLOCK
    } phy_state_t;

    typedef struct packed {
        sdspi_data_kind_t kind;
        logic card_present;
        logic read_only;
        logic card_idle;
        logic [31:0] resp0;
        logic [31:0] resp1;
        logic [31:0] data;
        logic last;
        logic error;
        logic [31:0] cmd_error;
        logic [31:0] data_error;
    } data_pkt_t;

    phy_state_t state;
    logic enabled;
    logic initialized;
    logic [31:0] clock_div;
    logic status_pending;
    logic [5:0] opcode;
    logic [31:0] argument;
    logic [2:0] rsp_type;
    logic data_present;
    logic data_write;
    logic [31:0] block_size;
    logic [31:0] block_count;
    logic [31:0] data_len;
    logic [31:0] total_offset;
    logic [31:0] block_offset;
    logic [31:0] word_buf;
    logic [1:0] byte_lane;
    logic [2:0] cmd_byte_idx;
    logic [2:0] extra_left;
    logic [1:0] crc_byte_idx;
    logic [31:0] response0;
    logic [31:0] response1;
    logic [31:0] timeout_count;
    logic [3:0] init_byte_count;
    logic multi_block;
    logic crc_enabled;
    logic [15:0] data_crc;
    logic [7:0] read_crc_high;

    logic data_vld;
    data_pkt_t data_pkt;

    logic byte_active;
    logic byte_start;
    logic [7:0] byte_tx;
    logic [7:0] byte_tx_shift;
    logic [7:0] byte_rx_shift;
    logic [7:0] byte_rx;
    logic [2:0] bit_count;
    logic [31:0] divider_count;
    logic byte_done;
    logic [31:0] half_period_cycles;

    function automatic logic [7:0] command_crc7;
        logic [6:0] crc;
        logic [7:0] value;
        crc = 0;
        for (int byte_idx = 0; byte_idx < 5; byte_idx++) begin
            unique case (byte_idx)
                0: value = {2'b01, opcode};
                1: value = argument[31:24];
                2: value = argument[23:16];
                3: value = argument[15:8];
                default: value = argument[7:0];
            endcase
            for (int bit_idx = 7; bit_idx >= 0; bit_idx--) begin
                logic feedback;
                feedback = crc[6] ^ value[bit_idx];
                crc = {crc[5:0], 1'b0};
                if (feedback) crc ^= 7'h09;
            end
        end
        return {crc, 1'b1};
    endfunction

    function automatic logic [15:0] data_crc16_byte(
        input logic [15:0] crc_in,
        input logic [7:0] value
    );
        logic [15:0] crc;
        crc = crc_in;
        for (int bit_idx = 7; bit_idx >= 0; bit_idx--) begin
            logic feedback;
            feedback = crc[15] ^ value[bit_idx];
            crc = {crc[14:0], 1'b0};
            if (feedback) crc ^= 16'h1021;
        end
        return crc;
    endfunction

    function automatic logic [7:0] command_byte(input logic [2:0] index);
        unique case (index)
            0: command_byte = {2'b01, opcode};
            1: command_byte = argument[31:24];
            2: command_byte = argument[23:16];
            3: command_byte = argument[15:8];
            4: command_byte = argument[7:0];
            default: command_byte = command_crc7();
        endcase
    endfunction

    function automatic logic [2:0] response_extra_bytes(
        input logic [2:0] response_type
    );
        unique case (response_type)
            3: response_extra_bytes = 1;
            4, 5: response_extra_bytes = 4;
            default: response_extra_bytes = 0;
        endcase
    endfunction

    task automatic prepare_error(
        input logic [31:0] cmd_error,
        input logic [31:0] data_error
    );
        begin
            data_pkt <= '0;
            data_pkt.kind <= data_write ? SDSPI_DATA_KIND_WRITE_DONE :
                SDSPI_DATA_KIND_RESPONSE;
            data_pkt.card_present <= 1'b1;
            data_pkt.card_idle <= 1'b1;
            data_pkt.error <= 1'b1;
            data_pkt.cmd_error <= cmd_error;
            data_pkt.data_error <= data_error;
            state <= data_write ? PHY_SEND_WRITE_DONE : PHY_SEND_RESPONSE;
        end
    endtask

    assign cmd_slv.rdy = !status_pending && !data_vld &&
        (state == PHY_IDLE || state == PHY_WRITE_WAIT_WORD);
    assign data_mst.vld = data_vld;
    assign data_mst.pkt = data_pkt;
    assign spi_mosi = byte_active ? byte_tx_shift[7] : 1'b1;
    assign half_period_cycles = clock_div <= 2 ? 32'd1 : clock_div >> 1;

    always_comb begin
        byte_start = 1'b0;
        byte_tx = 8'hff;
        if (!byte_active && !byte_done && !data_vld) begin
            unique case (state)
                PHY_INIT_CLOCKS: byte_start = 1'b1;
                PHY_CMD_SEND: begin
                    byte_start = 1'b1;
                    byte_tx = command_byte(cmd_byte_idx);
                end
                PHY_CMD_WAIT_R1,
                PHY_CMD_SKIP_STUFF,
                PHY_CMD_READ_EXTRA,
                PHY_CMD_WAIT_BUSY,
                PHY_READ_WAIT_TOKEN,
                PHY_READ_BYTES,
                PHY_READ_CRC,
                PHY_WRITE_RESPONSE,
                PHY_WRITE_BUSY,
                PHY_WRITE_STOP_BUSY,
                PHY_END_CLOCK: byte_start = 1'b1;
                PHY_WRITE_TOKEN: begin
                    byte_start = 1'b1;
                    byte_tx = multi_block ? 8'hfc : 8'hfe;
                end
                PHY_WRITE_BYTES: begin
                    byte_start = 1'b1;
                    unique case (byte_lane)
                        0: byte_tx = word_buf[7:0];
                        1: byte_tx = word_buf[15:8];
                        2: byte_tx = word_buf[23:16];
                        default: byte_tx = word_buf[31:24];
                    endcase
                end
                PHY_WRITE_CRC: begin
                    byte_start = 1'b1;
                    byte_tx = crc_byte_idx == 0 ? data_crc[15:8] :
                        data_crc[7:0];
                end
                PHY_WRITE_STOP: begin
                    byte_start = 1'b1;
                    byte_tx = 8'hfd;
                end
                default: begin end
            endcase
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            byte_active <= 1'b0;
            byte_tx_shift <= 8'hff;
            byte_rx_shift <= 0;
            byte_rx <= 0;
            bit_count <= 0;
            divider_count <= 0;
            byte_done <= 1'b0;
            spi_clk <= 1'b0;
        end else begin
            byte_done <= 1'b0;
            if (byte_start) begin
                byte_active <= 1'b1;
                byte_tx_shift <= byte_tx;
                byte_rx_shift <= 0;
                bit_count <= 0;
                divider_count <= 0;
                spi_clk <= 1'b0;
            end else if (byte_active) begin
                if (divider_count + 1 >= half_period_cycles) begin
                    divider_count <= 0;
                    if (!spi_clk) begin
                        spi_clk <= 1'b1;
                        byte_rx_shift <= {byte_rx_shift[6:0], spi_miso};
                    end else begin
                        spi_clk <= 1'b0;
                        if (bit_count == 7) begin
                            byte_active <= 1'b0;
                            byte_done <= 1'b1;
                            byte_rx <= byte_rx_shift;
                        end else begin
                            bit_count <= bit_count + 1'b1;
                            byte_tx_shift <= {byte_tx_shift[6:0], 1'b1};
                        end
                    end
                end else begin
                    divider_count <= divider_count + 1'b1;
                end
            end else begin
                spi_clk <= 1'b0;
            end
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= PHY_IDLE;
            enabled <= 1'b0;
            initialized <= 1'b0;
            clock_div <= 32'd250;
            status_pending <= 1'b1;
            spi_cs_n <= 1'b1;
            data_vld <= 1'b0;
            data_pkt <= '0;
            opcode <= 0;
            argument <= 0;
            rsp_type <= 0;
            data_present <= 1'b0;
            data_write <= 1'b0;
            block_size <= 0;
            block_count <= 0;
            data_len <= 0;
            total_offset <= 0;
            block_offset <= 0;
            word_buf <= 0;
            byte_lane <= 0;
            cmd_byte_idx <= 0;
            extra_left <= 0;
            crc_byte_idx <= 0;
            response0 <= 0;
            response1 <= 0;
            timeout_count <= 0;
            init_byte_count <= 0;
            multi_block <= 1'b0;
            crc_enabled <= 1'b0;
            data_crc <= 0;
            read_crc_high <= 0;
        end else begin
            if (data_vld && data_mst.rdy) begin
                data_vld <= 1'b0;
                unique case (data_pkt.kind)
                    SDSPI_DATA_KIND_STATUS: status_pending <= 1'b0;
                    SDSPI_DATA_KIND_RESPONSE: begin
                        if (data_pkt.error || !data_present)
                            state <= PHY_END_CLOCK;
                        else if (data_write)
                            state <= PHY_WRITE_TOKEN;
                        else
                            state <= PHY_READ_WAIT_TOKEN;
                    end
                    SDSPI_DATA_KIND_READ_DATA: begin
                        if (block_offset == block_size)
                            state <= PHY_READ_CRC;
                        else
                            state <= PHY_READ_BYTES;
                    end
                    SDSPI_DATA_KIND_WRITE_DONE: state <= PHY_END_CLOCK;
                endcase
            end

            if (!data_vld && status_pending) begin
                data_vld <= 1'b1;
                data_pkt <= '0;
                data_pkt.kind <= SDSPI_DATA_KIND_STATUS;
                data_pkt.card_present <= 1'b1;
                data_pkt.card_idle <= 1'b1;
            end

            if (cmd_slv.vld && cmd_slv.rdy) begin
                unique case (cmd_slv.pkt.kind)
                    SDSPI_CMD_KIND_RESET: begin
                        enabled <= 1'b0;
                        initialized <= 1'b0;
                        crc_enabled <= 1'b0;
                        status_pending <= 1'b1;
                        spi_cs_n <= 1'b1;
                        state <= PHY_IDLE;
                    end
                    SDSPI_CMD_KIND_CONFIG: begin
                        enabled <= cmd_slv.pkt.enable;
                        clock_div <= cmd_slv.pkt.clock_div == 0 ? 1 :
                            cmd_slv.pkt.clock_div;
                        if (cmd_slv.pkt.enable && !initialized) begin
                            init_byte_count <= 0;
                            spi_cs_n <= 1'b1;
                            state <= PHY_INIT_CLOCKS;
                        end
                    end
                    SDSPI_CMD_KIND_COMMAND: begin
                        opcode <= cmd_slv.pkt.opcode;
                        argument <= cmd_slv.pkt.arg;
                        rsp_type <= cmd_slv.pkt.rsp_type;
                        data_present <= cmd_slv.pkt.data_present;
                        data_write <= cmd_slv.pkt.write;
                        block_size <= cmd_slv.pkt.block_size;
                        block_count <= cmd_slv.pkt.block_count;
                        data_len <= cmd_slv.pkt.data_len;
                        total_offset <= 0;
                        block_offset <= 0;
                        byte_lane <= 0;
                        cmd_byte_idx <= 0;
                        response0 <= 0;
                        response1 <= 0;
                        timeout_count <= 0;
                        multi_block <= cmd_slv.pkt.block_count > 1;
                        spi_cs_n <= 1'b0;
                        state <= PHY_CMD_SEND;
                    end
                    SDSPI_CMD_KIND_WRITE_DATA: begin
                        word_buf <= cmd_slv.pkt.data;
                        byte_lane <= 0;
                        state <= PHY_WRITE_BYTES;
                    end
                endcase
            end

            if (byte_done) begin
                unique case (state)
                    PHY_INIT_CLOCKS: begin
                        if (init_byte_count == 9) begin
                            initialized <= 1'b1;
                            state <= PHY_IDLE;
                        end else init_byte_count <= init_byte_count + 1'b1;
                    end
                    PHY_CMD_SEND: begin
                        if (cmd_byte_idx == 5) begin
                            timeout_count <= 0;
                            state <= opcode == 12 ? PHY_CMD_SKIP_STUFF :
                                PHY_CMD_WAIT_R1;
                        end else cmd_byte_idx <= cmd_byte_idx + 1'b1;
                    end
                    PHY_CMD_SKIP_STUFF: state <= PHY_CMD_WAIT_R1;
                    PHY_CMD_WAIT_R1: begin
                        if (!byte_rx[7]) begin
                            if (opcode == 0)
                                crc_enabled <= 1'b0;
                            else if (opcode == 59 &&
                                (byte_rx & 8'h7e) == 0)
                                crc_enabled <= argument[0];
                            response0 <= {24'b0, byte_rx};
                            response1 <= 0;
                            extra_left <= response_extra_bytes(rsp_type);
                            if (response_extra_bytes(rsp_type) != 0)
                                state <= PHY_CMD_READ_EXTRA;
                            else if (rsp_type == 2)
                                state <= PHY_CMD_WAIT_BUSY;
                            else
                                state <= PHY_SEND_RESPONSE;
                        end else if (timeout_count ==
                            RESPONSE_TIMEOUT_BYTES - 1) begin
                            prepare_error(`SDSPI_CMD_STATUS_TIMEOUT, 0);
                        end else timeout_count <= timeout_count + 1'b1;
                    end
                    PHY_CMD_READ_EXTRA: begin
                        response1 <= {response1[23:0], byte_rx};
                        if (extra_left == 1) begin
                            if (rsp_type == 2)
                                state <= PHY_CMD_WAIT_BUSY;
                            else
                                state <= PHY_SEND_RESPONSE;
                        end else extra_left <= extra_left - 1'b1;
                    end
                    PHY_CMD_WAIT_BUSY: begin
                        if (byte_rx == 8'hff)
                            state <= PHY_SEND_RESPONSE;
                        else if (timeout_count == DATA_TIMEOUT_BYTES - 1)
                            prepare_error(`SDSPI_CMD_STATUS_TIMEOUT, 0);
                        else timeout_count <= timeout_count + 1'b1;
                    end
                    PHY_READ_WAIT_TOKEN: begin
                        if (byte_rx == 8'hfe) begin
                            block_offset <= 0;
                            byte_lane <= 0;
                            data_crc <= 0;
                            timeout_count <= 0;
                            state <= PHY_READ_BYTES;
                        end else if (timeout_count == DATA_TIMEOUT_BYTES - 1)
                            prepare_error(0, `SDSPI_DATA_STATUS_IO_ERROR);
                        else timeout_count <= timeout_count + 1'b1;
                    end
                    PHY_READ_BYTES: begin
                        data_crc <= data_crc16_byte(data_crc, byte_rx);
                        word_buf[byte_lane * 8 +: 8] <= byte_rx;
                        total_offset <= total_offset + 1'b1;
                        block_offset <= block_offset + 1'b1;
                        if (byte_lane == 3) begin
                            byte_lane <= 0;
                            data_pkt <= '0;
                            data_pkt.kind <= SDSPI_DATA_KIND_READ_DATA;
                            data_pkt.card_present <= 1'b1;
                            data_pkt.card_idle <= response0[0];
                            data_pkt.data <= {byte_rx, word_buf[23:0]};
                            data_pkt.last <= total_offset + 1 == data_len;
                            state <= PHY_READ_SEND_WORD;
                        end else byte_lane <= byte_lane + 1'b1;
                    end
                    PHY_READ_CRC: begin
                        if (crc_byte_idx == 1) begin
                            crc_byte_idx <= 0;
                            if (crc_enabled &&
                                {read_crc_high, byte_rx} != data_crc)
                                prepare_error(0,
                                    `SDSPI_DATA_STATUS_IO_ERROR);
                            else if (total_offset == data_len)
                                state <= multi_block ? PHY_IDLE : PHY_END_CLOCK;
                            else
                                state <= PHY_READ_WAIT_TOKEN;
                        end else begin
                            read_crc_high <= byte_rx;
                            crc_byte_idx <= crc_byte_idx + 1'b1;
                        end
                    end
                    PHY_WRITE_TOKEN: begin
                        byte_lane <= 0;
                        block_offset <= 0;
                        data_crc <= 0;
                        state <= PHY_WRITE_WAIT_WORD;
                    end
                    PHY_WRITE_BYTES: begin
                        data_crc <= data_crc16_byte(data_crc,
                            word_buf[byte_lane * 8 +: 8]);
                        total_offset <= total_offset + 1'b1;
                        block_offset <= block_offset + 1'b1;
                        if (byte_lane == 3) begin
                            byte_lane <= 0;
                            if (block_offset + 1 == block_size) begin
                                crc_byte_idx <= 0;
                                state <= PHY_WRITE_CRC;
                            end else begin
                                state <= PHY_WRITE_WAIT_WORD;
                            end
                        end else byte_lane <= byte_lane + 1'b1;
                    end
                    PHY_WRITE_CRC: begin
                        if (crc_byte_idx == 1) begin
                            crc_byte_idx <= 0;
                            timeout_count <= 0;
                            state <= PHY_WRITE_RESPONSE;
                        end else crc_byte_idx <= crc_byte_idx + 1'b1;
                    end
                    PHY_WRITE_RESPONSE: begin
                        if (byte_rx[4:0] == 5'b00101) begin
                            timeout_count <= 0;
                            state <= PHY_WRITE_BUSY;
                        end else if (timeout_count ==
                            RESPONSE_TIMEOUT_BYTES - 1) begin
                            prepare_error(0, `SDSPI_DATA_STATUS_IO_ERROR);
                        end else timeout_count <= timeout_count + 1'b1;
                    end
                    PHY_WRITE_BUSY: begin
                        if (byte_rx == 8'hff) begin
                            if (total_offset == data_len) begin
                                if (multi_block)
                                    state <= PHY_WRITE_STOP;
                                else
                                    state <= PHY_SEND_WRITE_DONE;
                            end else begin
                                state <= PHY_WRITE_TOKEN;
                            end
                        end else if (timeout_count == DATA_TIMEOUT_BYTES - 1)
                            prepare_error(0, `SDSPI_DATA_STATUS_IO_ERROR);
                        else timeout_count <= timeout_count + 1'b1;
                    end
                    PHY_WRITE_STOP: begin
                        timeout_count <= 0;
                        state <= PHY_WRITE_STOP_BUSY;
                    end
                    PHY_WRITE_STOP_BUSY: begin
                        if (byte_rx == 8'hff)
                            state <= PHY_SEND_WRITE_DONE;
                        else if (timeout_count == DATA_TIMEOUT_BYTES - 1)
                            prepare_error(0, `SDSPI_DATA_STATUS_IO_ERROR);
                        else timeout_count <= timeout_count + 1'b1;
                    end
                    PHY_END_CLOCK: begin
                        spi_cs_n <= 1'b1;
                        state <= PHY_IDLE;
                    end
                    default: begin end
                endcase
            end

            if (!data_vld && !status_pending) begin
                if (state == PHY_SEND_RESPONSE) begin
                    data_vld <= 1'b1;
                    data_pkt <= '0;
                    data_pkt.kind <= SDSPI_DATA_KIND_RESPONSE;
                    data_pkt.card_present <= 1'b1;
                    data_pkt.card_idle <= response0[0];
                    data_pkt.resp0 <= response0;
                    data_pkt.resp1 <= response1;
                end else if (state == PHY_READ_SEND_WORD) begin
                    data_vld <= 1'b1;
                end else if (state == PHY_SEND_WRITE_DONE) begin
                    data_vld <= 1'b1;
                    data_pkt <= '0;
                    data_pkt.kind <= SDSPI_DATA_KIND_WRITE_DONE;
                    data_pkt.card_present <= 1'b1;
                    data_pkt.card_idle <= response0[0];
                end
            end

            if (state == PHY_END_CLOCK)
                spi_cs_n <= 1'b1;
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && cmd_slv.vld && cmd_slv.rdy &&
            cmd_slv.pkt.kind == SDSPI_CMD_KIND_COMMAND)
            assert (enabled && initialized)
                else $fatal(1, "sdspi_phy command while disabled");
    end
`endif

    wire unused = |block_count;
endmodule
