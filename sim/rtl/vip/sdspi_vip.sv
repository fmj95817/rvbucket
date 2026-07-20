`include "spec/io/sdspi.svh"
`include "itf/sdspi_cmd_if.svh"
`include "itf/sdspi_data_if.svh"

import "DPI-C" function int sdspi_backend_open(input string path);
import "DPI-C" function longint unsigned sdspi_backend_size();
import "DPI-C" function int sdspi_backend_read_only();
import "DPI-C" function int sdspi_backend_read_word(
    input longint unsigned offset, output int unsigned data);
import "DPI-C" function int sdspi_backend_write_word(
    input longint unsigned offset, input int unsigned data);
import "DPI-C" function void sdspi_backend_close();

module sdspi_vip(
    input logic clk,
    input logic rst_n,
    sdspi_cmd_if_t.slv cmd_slv,
    sdspi_data_if_t.mst data_mst
);
    localparam logic [31:0] R1_IDLE = 32'h01;
    localparam logic [31:0] R1_ILLEGAL = 32'h04;
    localparam logic [31:0] R1_ADDRESS = 32'h20;
    localparam logic [31:0] R1_PARAMETER = 32'h40;
    localparam logic [31:0] OCR = 32'hc0ff8000;

    typedef enum logic [2:0] {
        VIP_IDLE,
        VIP_SEND_RESPONSE,
        VIP_SEND_READ_DATA,
        VIP_RECV_WRITE_DATA,
        VIP_SEND_WRITE_DONE
    } vip_state_t;

    typedef enum logic [2:0] {
        DATA_REGULAR,
        DATA_ZERO,
        DATA_SCR,
        DATA_CID,
        DATA_CSD
    } data_source_t;

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

    vip_state_t state;
    data_source_t data_source;
    logic card_present;
    logic read_only;
    logic card_idle;
    logic app_cmd;
    logic enabled;
    logic [31:0] clock_div;
    logic status_pending;
    logic data_write;
    logic [31:0] data_len;
    logic [31:0] data_offset;
    logic [63:0] image_offset;
    logic [63:0] image_size;
    logic data_vld;
    data_pkt_t data_pkt;
    string image_path;
    int image_arg_valid;

    function automatic logic [7:0] cid_byte(input logic [4:0] index);
        unique case (index)
            0: cid_byte = 8'h52;
            1: cid_byte = "R";
            2: cid_byte = "V";
            3: cid_byte = "R";
            4: cid_byte = "V";
            5: cid_byte = "B";
            6: cid_byte = "S";
            7: cid_byte = "D";
            8: cid_byte = 8'h10;
            9: cid_byte = 8'h12;
            10: cid_byte = 8'h34;
            11: cid_byte = 8'h56;
            12: cid_byte = 8'h78;
            13: cid_byte = 8'h02;
            14: cid_byte = 8'h67;
            default: cid_byte = 8'h01;
        endcase
    endfunction

    function automatic logic [7:0] csd_byte(input logic [4:0] index);
        logic [21:0] csize;
        logic [63:0] units;
        logic [63:0] csize_long;
        units = image_size / (512 * 1024);
        csize_long = units == 0 ? 0 : units - 1;
        csize = csize_long > 64'h3fffff ? 22'h3fffff :
            csize_long[21:0];
        unique case (index)
            0: csd_byte = 8'h40;
            1: csd_byte = 8'h0e;
            2: csd_byte = 8'h00;
            3: csd_byte = 8'h32;
            4: csd_byte = 8'h5b;
            5: csd_byte = 8'h59;
            6: csd_byte = 8'h00;
            7: csd_byte = {2'b0, csize[21:16]};
            8: csd_byte = csize[15:8];
            9: csd_byte = csize[7:0];
            10: csd_byte = 8'h7f;
            11: csd_byte = 8'h80;
            12: csd_byte = 8'h0a;
            13: csd_byte = 8'h40;
            14: csd_byte = 8'h00;
            default: csd_byte = 8'h01;
        endcase
    endfunction

    function automatic logic [7:0] special_byte(input logic [31:0] offset);
        unique case (data_source)
            DATA_SCR: begin
                if (offset == 0) special_byte = 8'h02;
                else if (offset == 1) special_byte = 8'h25;
                else special_byte = 0;
            end
            DATA_CID: special_byte = cid_byte(offset[4:0]);
            DATA_CSD: special_byte = csd_byte(offset[4:0]);
            default: special_byte = 0;
        endcase
    endfunction

    function automatic logic [31:0] special_word(input logic [31:0] offset);
        return {special_byte(offset + 3), special_byte(offset + 2),
            special_byte(offset + 1), special_byte(offset)};
    endfunction

    task automatic fill_card_status;
        begin
            data_pkt.card_present <= card_present;
            data_pkt.read_only <= read_only;
            data_pkt.card_idle <= card_idle;
        end
    endtask

    task automatic protocol_reset;
        begin
            card_idle <= 1'b1;
            app_cmd <= 1'b0;
            state <= VIP_IDLE;
            data_source <= DATA_ZERO;
            data_len <= 0;
            data_offset <= 0;
            image_offset <= 0;
            data_write <= 1'b0;
            status_pending <= 1'b1;
            enabled <= 1'b0;
            clock_div <= 0;
            data_vld <= 1'b0;
            data_pkt <= '0;
        end
    endtask

    assign cmd_slv.rdy = !status_pending && !data_vld &&
        (state == VIP_IDLE || state == VIP_RECV_WRITE_DATA);
    assign data_mst.vld = data_vld;
    assign data_mst.pkt = data_pkt;

    initial begin
        image_arg_valid = $value$plusargs("sd_image=%s", image_path);
        card_present = image_arg_valid != 0 &&
            sdspi_backend_open(image_path) != 0;
        image_size = card_present ? sdspi_backend_size() : 0;
        read_only = card_present && sdspi_backend_read_only() != 0;
        if (image_arg_valid != 0 && !card_present)
            $fatal(1, "sdspi_vip: cannot open usable image '%s'", image_path);
        if (card_present)
            $display("sdspi_vip: image '%s', %0d bytes%s", image_path,
                image_size, read_only ? " (read-only)" : "");
    end

    final begin
        sdspi_backend_close();
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            protocol_reset();
        end else begin
            if (data_vld && data_mst.rdy) begin
                data_vld <= 1'b0;
                unique case (data_pkt.kind)
                    SDSPI_DATA_KIND_STATUS: status_pending <= 1'b0;
                    SDSPI_DATA_KIND_RESPONSE: begin
                        if (data_pkt.error || data_len == 0)
                            state <= VIP_IDLE;
                        else if (data_write)
                            state <= VIP_RECV_WRITE_DATA;
                        else
                            state <= VIP_SEND_READ_DATA;
                    end
                    SDSPI_DATA_KIND_READ_DATA: begin
                        data_offset <= data_offset + 4;
                        if (data_pkt.last) state <= VIP_IDLE;
                    end
                    SDSPI_DATA_KIND_WRITE_DONE: state <= VIP_IDLE;
                endcase
            end

            if (!data_vld) begin
                if (status_pending) begin
                    data_vld <= 1'b1;
                    data_pkt <= '0;
                    data_pkt.kind <= SDSPI_DATA_KIND_STATUS;
                    fill_card_status();
                end else if (state == VIP_SEND_RESPONSE) begin
                    data_vld <= 1'b1;
                end else if (state == VIP_SEND_READ_DATA) begin
                    logic [31:0] value;
                    logic read_ok;
                    value = 0;
                    read_ok = 1;
                    if (data_source == DATA_REGULAR)
                        read_ok = sdspi_backend_read_word(
                            image_offset + {32'b0, data_offset}, value) != 0;
                    else
                        value = special_word(data_offset);
                    data_vld <= 1'b1;
                    data_pkt <= '0;
                    data_pkt.kind <= SDSPI_DATA_KIND_READ_DATA;
                    data_pkt.data <= value;
                    data_pkt.last <= data_offset + 4 == data_len;
                    data_pkt.error <= !read_ok;
                    data_pkt.data_error <= read_ok ? 0 :
                        `SDSPI_DATA_STATUS_IO_ERROR;
                    fill_card_status();
                end else if (state == VIP_SEND_WRITE_DONE) begin
                    data_vld <= 1'b1;
                end
            end

            if (cmd_slv.vld && cmd_slv.rdy) begin
                if (cmd_slv.pkt.kind == SDSPI_CMD_KIND_RESET) begin
                    protocol_reset();
                end else if (cmd_slv.pkt.kind == SDSPI_CMD_KIND_CONFIG) begin
                    enabled <= cmd_slv.pkt.enable;
                    clock_div <= cmd_slv.pkt.clock_div;
                end else if (cmd_slv.pkt.kind == SDSPI_CMD_KIND_COMMAND) begin
                    logic [31:0] cmd_error;
                    logic [31:0] response0;
                    logic [31:0] response1;
                    logic [63:0] length;
                    logic valid_shape;
                    logic was_app_cmd;
                    cmd_error = 0;
                    response0 = card_idle ? R1_IDLE : 0;
                    response1 = 0;
                    length = cmd_slv.pkt.block_size *
                        cmd_slv.pkt.block_count;
                    valid_shape = (!cmd_slv.pkt.data_present &&
                        cmd_slv.pkt.data_len == 0 && !cmd_slv.pkt.write) ||
                        (cmd_slv.pkt.data_present &&
                         cmd_slv.pkt.data_len != 0 &&
                         cmd_slv.pkt.data_len <= `SDSPI_MAX_DATA_SIZE &&
                         cmd_slv.pkt.data_len[1:0] == 0);
                    data_len <= cmd_slv.pkt.data_present ?
                        cmd_slv.pkt.data_len : 0;
                    data_write <= cmd_slv.pkt.data_present &&
                        cmd_slv.pkt.write;
                    data_offset <= 0;
                    data_source <= DATA_ZERO;
                    was_app_cmd = app_cmd && cmd_slv.pkt.opcode != 55;
                    if (was_app_cmd) app_cmd <= 1'b0;

                    if (!enabled || !card_present)
                        cmd_error = `SDSPI_CMD_STATUS_TIMEOUT;
                    else if (!valid_shape)
                        cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                    else if (was_app_cmd) begin
                        unique case (cmd_slv.pkt.opcode)
                            13: begin
                                if (cmd_slv.pkt.data_len != 64 ||
                                    cmd_slv.pkt.write)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                                else data_source <= DATA_ZERO;
                            end
                            23: if (cmd_slv.pkt.data_len != 0)
                                cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                            41: begin
                                if (cmd_slv.pkt.data_len != 0)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                                else begin
                                    card_idle <= 1'b0;
                                    response0 = 0;
                                end
                            end
                            51: begin
                                if (cmd_slv.pkt.data_len != 8 ||
                                    cmd_slv.pkt.write)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                                else data_source <= DATA_SCR;
                            end
                            default: begin
                                cmd_error = `SDSPI_CMD_STATUS_ILLEGAL;
                                response0 |= R1_ILLEGAL;
                            end
                        endcase
                    end else begin
                        unique case (cmd_slv.pkt.opcode)
                            0: begin
                                card_idle <= 1'b1;
                                app_cmd <= 1'b0;
                                response0 = R1_IDLE;
                                if (cmd_slv.pkt.data_len != 0)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                            end
                            6: begin
                                if (cmd_slv.pkt.data_len != 64 ||
                                    cmd_slv.pkt.write)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                                else data_source <= DATA_ZERO;
                            end
                            8: begin
                                response1 = cmd_slv.pkt.arg;
                                if (cmd_slv.pkt.data_len != 0)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                            end
                            9: begin
                                if (cmd_slv.pkt.data_len != 16 ||
                                    cmd_slv.pkt.write)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                                else data_source <= DATA_CSD;
                            end
                            10: begin
                                if (cmd_slv.pkt.data_len != 16 ||
                                    cmd_slv.pkt.write)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                                else data_source <= DATA_CID;
                            end
                            12, 13, 23, 59: begin
                                if (cmd_slv.pkt.data_len != 0)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                            end
                            16: begin
                                if (cmd_slv.pkt.arg == 0 ||
                                    cmd_slv.pkt.arg > `SDSPI_SECTOR_SIZE) begin
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                                    response0 |= R1_PARAMETER;
                                end
                            end
                            17, 18, 24, 25: begin
                                image_offset <= cmd_slv.pkt.arg * 64'd512;
                                if (length != {32'b0,
                                        cmd_slv.pkt.data_len} ||
                                    length == 0 ||
                                    (cmd_slv.pkt.opcode == 17 ||
                                     cmd_slv.pkt.opcode == 24) &&
                                        cmd_slv.pkt.block_count != 1 ||
                                    cmd_slv.pkt.write !=
                                        (cmd_slv.pkt.opcode == 24 ||
                                         cmd_slv.pkt.opcode == 25) ||
                                    cmd_slv.pkt.arg * 64'd512 > image_size ||
                                    length > image_size -
                                        cmd_slv.pkt.arg * 64'd512 ||
                                    (cmd_slv.pkt.write && read_only)) begin
                                    cmd_error = `SDSPI_CMD_STATUS_ADDRESS;
                                    response0 |= R1_ADDRESS;
                                end else data_source <= DATA_REGULAR;
                            end
                            55: begin
                                app_cmd <= 1'b1;
                                if (cmd_slv.pkt.data_len != 0)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                            end
                            58: begin
                                response1 = OCR;
                                if (cmd_slv.pkt.data_len != 0)
                                    cmd_error = `SDSPI_CMD_STATUS_PARAMETER;
                            end
                            default: begin
                                cmd_error = `SDSPI_CMD_STATUS_ILLEGAL;
                                response0 |= R1_ILLEGAL;
                            end
                        endcase
                    end
                    data_pkt <= '0;
                    data_pkt.kind <= SDSPI_DATA_KIND_RESPONSE;
                    data_pkt.resp0 <= response0;
                    data_pkt.resp1 <= response1;
                    data_pkt.error <= cmd_error != 0;
                    data_pkt.cmd_error <= cmd_error;
                    fill_card_status();
                    state <= VIP_SEND_RESPONSE;
                end else if (cmd_slv.pkt.kind == SDSPI_CMD_KIND_WRITE_DATA) begin
                    logic expected_last;
                    logic write_ok;
                    expected_last = data_offset + 4 == data_len;
                    write_ok = state == VIP_RECV_WRITE_DATA &&
                        data_offset + 4 <= data_len &&
                        cmd_slv.pkt.last == expected_last &&
                        sdspi_backend_write_word(image_offset +
                            {32'b0, data_offset}, cmd_slv.pkt.data) != 0;
                    data_offset <= data_offset + 4;
                    if (!write_ok || cmd_slv.pkt.last) begin
                        data_pkt <= '0;
                        data_pkt.kind <= SDSPI_DATA_KIND_WRITE_DONE;
                        data_pkt.error <= !write_ok;
                        data_pkt.data_error <= write_ok ? 0 :
                            `SDSPI_DATA_STATUS_IO_ERROR;
                        fill_card_status();
                        state <= VIP_SEND_WRITE_DONE;
                    end
                end
            end
        end
    end

    wire unused = |clock_div;
endmodule
