`timescale 1ns/1ps

`include "itf/sdspi_cmd_if.svh"
`include "itf/sdspi_data_if.svh"

module sdspi_phy_card_model(
    input logic spi_clk,
    input logic spi_cs_n,
    input logic spi_mosi,
    output logic spi_miso
);
    logic [7:0] command[0:5];
    logic [7:0] tx_data[0:2047];
    logic [7:0] write_data[0:1023];
    logic [7:0] rx_shift;
    integer rx_bit;
    integer command_byte;
    integer tx_len;
    integer tx_byte;
    integer tx_bit;
    integer write_offset;
    integer write_block_offset;
    integer write_crc_left;
    logic write_wait_token;
    logic write_receiving;
    logic write_multi;
    logic command_crc_ok;
    logic crc_enabled;
    logic [15:0] write_crc;
    logic [15:0] write_crc_rx;
    logic write_crc_ok;

    function automatic logic [15:0] crc16_byte(
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

    task automatic prepare_response(input logic [5:0] opcode);
        logic [31:0] argument;
        logic [15:0] crc;
        begin
            $display("card command: %02x %02x %02x %02x %02x %02x",
                command[0], command[1], command[2], command[3], command[4],
                command[5]);
            tx_byte = 0;
            tx_bit = 0;
            argument = {command[1], command[2], command[3], command[4]};
            unique case (opcode)
                0: begin
                    command_crc_ok = command_crc_ok && command[5] == 8'h95;
                    crc_enabled = 1'b0;
                    tx_data[0] = 8'h01;
                    tx_len = 1;
                end
                8: begin
                    command_crc_ok = command_crc_ok && command[5] == 8'h87;
                    tx_data[0] = 8'h01;
                    tx_data[1] = 8'h00;
                    tx_data[2] = 8'h00;
                    tx_data[3] = 8'h01;
                    tx_data[4] = 8'haa;
                    tx_len = 5;
                end
                17: begin
                    tx_data[0] = 8'h00;
                    tx_data[1] = 8'hfe;
                    crc = 0;
                    for (int i = 0; i < 512; i++) begin
                        if (argument == 32'd65535)
                            tx_data[i + 2] = write_data[i];
                        else if (argument == 32'd2048)
                            tx_data[i + 2] = 8'h00;
                        else if (i == 510)
                            tx_data[i + 2] = 8'h55;
                        else if (i == 511)
                            tx_data[i + 2] = 8'haa;
                        else
                            tx_data[i + 2] = i[7:0];
                    end
                    if (argument == 32'd2048) begin
                        tx_data[2] = 8'h01;
                        tx_data[6] = 8'h40;
                    end
                    crc = 0;
                    for (int i = 0; i < 512; i++)
                        crc = crc16_byte(crc, tx_data[i + 2]);
                    tx_data[514] = crc[15:8];
                    tx_data[515] = crc[7:0];
                    tx_len = 516;
                end
                18: begin
                    tx_data[0] = 8'h00;
                    for (int block_idx = 0; block_idx < 2; block_idx++) begin
                        int base;
                        base = 1 + block_idx * 515;
                        tx_data[base] = 8'hfe;
                        crc = 0;
                        for (int i = 0; i < 512; i++) begin
                            if (argument == 32'd65534)
                                tx_data[base + i + 1] =
                                    write_data[block_idx * 512 + i];
                            else if (i == 510)
                                tx_data[base + i + 1] = 8'h55;
                            else if (i == 511)
                                tx_data[base + i + 1] = 8'haa;
                            else
                                tx_data[base + i + 1] = i[7:0];
                            crc = crc16_byte(crc,
                                tx_data[base + i + 1]);
                        end
                        tx_data[base + 513] = crc[15:8];
                        tx_data[base + 514] = crc[7:0];
                    end
                    tx_len = 1031;
                end
                12: begin
                    tx_data[0] = 8'hff;
                    tx_data[1] = 8'h00;
                    tx_data[2] = 8'hff;
                    tx_len = 3;
                end
                24: begin
                    tx_data[0] = 8'h00;
                    tx_len = 1;
                    write_wait_token = 1'b1;
                    write_offset = 0;
                    write_block_offset = 0;
                    write_crc_left = 0;
                    write_multi = 1'b0;
                end
                25: begin
                    tx_data[0] = 8'h00;
                    tx_len = 1;
                    write_wait_token = 1'b1;
                    write_offset = 0;
                    write_block_offset = 0;
                    write_crc_left = 0;
                    write_multi = 1'b1;
                end
                41: begin
                    tx_data[0] = 8'h00;
                    tx_len = 1;
                end
                55: begin
                    tx_data[0] = 8'h01;
                    tx_len = 1;
                end
                58: begin
                    tx_data[0] = 8'h00;
                    tx_data[1] = 8'hc0;
                    tx_data[2] = 8'hff;
                    tx_data[3] = 8'h80;
                    tx_data[4] = 8'h00;
                    tx_len = 5;
                end
                59: begin
                    crc_enabled = argument[0];
                    tx_data[0] = 8'h00;
                    tx_len = 1;
                end
                default: begin
                    tx_data[0] = 8'h04;
                    tx_len = 1;
                end
            endcase
        end
    endtask

    task automatic receive_byte(input logic [7:0] value);
        begin
            if (command_byte == 6 && !write_wait_token && !write_receiving &&
                tx_byte >= tx_len && value[7:6] == 2'b01) begin
                command[0] = value;
                command_byte = 1;
            end else if (command_byte < 6) begin
                command[command_byte] = value;
                command_byte = command_byte + 1;
                if (command_byte == 6)
                    prepare_response(command[0][5:0]);
            end else if (write_wait_token &&
                (value == 8'hfe || (write_multi && value == 8'hfc))) begin
                write_wait_token = 1'b0;
                write_receiving = 1'b1;
                write_block_offset = 0;
                write_crc = 0;
                write_crc_rx = 0;
            end else if (write_wait_token && write_multi && value == 8'hfd) begin
                write_wait_token = 1'b0;
                write_multi = 1'b0;
            end else if (write_receiving) begin
                if (write_block_offset < 512) begin
                    write_data[write_offset] = value;
                    write_crc = crc16_byte(write_crc, value);
                    write_offset = write_offset + 1;
                    write_block_offset = write_block_offset + 1;
                    if (write_block_offset == 512)
                        write_crc_left = 2;
                end else if (write_crc_left != 0) begin
                    write_crc_rx = {write_crc_rx[7:0], value};
                    write_crc_left = write_crc_left - 1;
                    if (write_crc_left == 0) begin
                        write_crc_ok = write_crc_ok &&
                            (!crc_enabled || write_crc_rx == write_crc);
                        write_receiving = 1'b0;
                        write_wait_token = write_multi;
                        tx_data[0] = !crc_enabled ||
                            write_crc_rx == write_crc ? 8'h05 : 8'h0b;
                        tx_data[1] = 8'h00;
                        tx_data[2] = 8'h00;
                        tx_data[3] = 8'hff;
                        tx_len = 4;
                        tx_byte = 0;
                        tx_bit = 0;
                    end
                end
            end
        end
    endtask

    initial begin
        spi_miso = 1'b1;
        rx_shift = 0;
        rx_bit = 0;
        command_byte = 0;
        tx_len = 0;
        tx_byte = 0;
        tx_bit = 0;
        write_offset = 0;
        write_block_offset = 0;
        write_crc_left = 0;
        write_wait_token = 1'b0;
        write_receiving = 1'b0;
        write_multi = 1'b0;
        command_crc_ok = 1'b1;
        crc_enabled = 1'b0;
        write_crc = 0;
        write_crc_rx = 0;
        write_crc_ok = 1'b1;
    end

    always @(posedge spi_cs_n) begin
        rx_shift = 0;
        rx_bit = 0;
        command_byte = 0;
        tx_len = 0;
        tx_byte = 0;
        tx_bit = 0;
        write_wait_token = 1'b0;
        write_receiving = 1'b0;
        write_multi = 1'b0;
        spi_miso = 1'b1;
    end

    always @(posedge spi_clk) begin
        if (!spi_cs_n) begin
            rx_shift = {rx_shift[6:0], spi_mosi};
            if (rx_bit == 7) begin
                receive_byte(rx_shift);
                rx_bit = 0;
                rx_shift = 0;
            end else begin
                rx_bit = rx_bit + 1;
            end
        end
    end

    always @(negedge spi_clk) begin
        if (!spi_cs_n && tx_byte < tx_len) begin
            spi_miso = tx_data[tx_byte][7 - tx_bit];
            if (tx_bit == 7) begin
                tx_bit = 0;
                tx_byte = tx_byte + 1;
            end else begin
                tx_bit = tx_bit + 1;
            end
        end else begin
            spi_miso = 1'b1;
        end
    end
endmodule

module sdspi_phy_tb;
    logic clk;
    logic rst_n;
    logic spi_clk;
    logic spi_cs_n;
    logic spi_mosi;
    logic spi_miso;
    sdspi_cmd_if_t cmd();
    sdspi_data_if_t data();

    sdspi_phy #(
        .RESPONSE_TIMEOUT_BYTES (32),
        .DATA_TIMEOUT_BYTES     (1024)
    ) u_dut(
        .clk      (clk),
        .rst_n    (rst_n),
        .cmd_slv  (cmd),
        .data_mst (data),
        .spi_clk  (spi_clk),
        .spi_cs_n (spi_cs_n),
        .spi_mosi (spi_mosi),
        .spi_miso (spi_miso)
    );

    sdspi_phy_card_model u_card(
        .spi_clk  (spi_clk),
        .spi_cs_n (spi_cs_n),
        .spi_mosi (spi_mosi),
        .spi_miso (spi_miso)
    );

    always #5 clk = !clk;

    function automatic logic [31:0] expected_read_word(input int word_idx);
        logic [7:0] first;
        if (word_idx == 127)
            return 32'haa55fdfc;
        first = 8'(word_idx * 4);
        return {first + 8'd3, first + 8'd2, first + 8'd1, first};
    endfunction

    task automatic send_packet;
        begin
            do @(negedge clk); while (!cmd.rdy);
            cmd.vld = 1'b1;
            @(negedge clk);
            cmd.vld = 1'b0;
            cmd.pkt = '0;
        end
    endtask

    task automatic configure;
        begin
            cmd.pkt = '0;
            cmd.pkt.kind = SDSPI_CMD_KIND_CONFIG;
            cmd.pkt.enable = 1'b1;
            cmd.pkt.clock_div = 4;
            send_packet();
        end
    endtask

    task automatic send_command(
        input logic [5:0] opcode,
        input logic [31:0] argument,
        input logic [2:0] rsp_type,
        input logic has_data,
        input logic write,
        input logic [31:0] blocks
    );
        begin
            cmd.pkt = '0;
            cmd.pkt.kind = SDSPI_CMD_KIND_COMMAND;
            cmd.pkt.opcode = opcode;
            cmd.pkt.arg = argument;
            cmd.pkt.rsp_type = rsp_type;
            cmd.pkt.data_present = has_data;
            cmd.pkt.write = write;
            cmd.pkt.block_size = has_data ? 512 : 0;
            cmd.pkt.block_count = has_data ? blocks : 0;
            cmd.pkt.data_len = has_data ? blocks * 512 : 0;
            send_packet();
        end
    endtask

    task automatic send_write_word(
        input logic [31:0] value,
        input logic last
    );
        begin
            cmd.pkt = '0;
            cmd.pkt.kind = SDSPI_CMD_KIND_WRITE_DATA;
            cmd.pkt.data = value;
            cmd.pkt.last = last;
            send_packet();
        end
    endtask

    task automatic wait_data_kind(input sdspi_data_kind_t kind);
        begin
            do @(posedge clk); while (!(data.vld && data.rdy &&
                data.pkt.kind == kind));
        end
    endtask

    initial begin
        logic [31:0] expected;
        clk = 1'b0;
        rst_n = 1'b0;
        cmd.vld = 1'b0;
        cmd.pkt = '0;
        data.rdy = 1'b1;
        repeat (5) @(posedge clk);
        rst_n = 1'b1;

        wait_data_kind(SDSPI_DATA_KIND_STATUS);
        assert (data.pkt.card_present) else $fatal(1, "card absent");
        configure();

        send_command(0, 0, 1, 1'b0, 1'b0, 0);
        wait_data_kind(SDSPI_DATA_KIND_RESPONSE);
        assert (data.pkt.resp0 == 1) else $fatal(1, "CMD0 response");

        send_command(8, 32'h000001aa, 5, 1'b0, 1'b0, 0);
        wait_data_kind(SDSPI_DATA_KIND_RESPONSE);
        $display("CMD8: resp0=%08x resp1=%08x", data.pkt.resp0,
            data.pkt.resp1);
        assert (data.pkt.resp0 == 1 && data.pkt.resp1 == 32'h000001aa)
            else $fatal(1, "CMD8 response");

        send_command(59, 1, 1, 1'b0, 1'b0, 0);
        wait_data_kind(SDSPI_DATA_KIND_RESPONSE);
        assert (!data.pkt.error && data.pkt.resp0 == 0)
            else $fatal(1, "CMD59 response");

        send_command(17, 0, 1, 1'b1, 1'b0, 1);
        wait_data_kind(SDSPI_DATA_KIND_RESPONSE);
        assert (data.pkt.resp0 == 0) else $fatal(1, "CMD17 response");
        for (int word_idx = 0; word_idx < 128; word_idx++) begin
            wait_data_kind(SDSPI_DATA_KIND_READ_DATA);
            expected = expected_read_word(word_idx);
            assert (data.pkt.data == expected)
                else $fatal(1, "read word %0d: %08x != %08x", word_idx,
                    data.pkt.data, expected);
            assert (data.pkt.last == (word_idx == 127))
                else $fatal(1, "read last %0d", word_idx);
        end

        send_command(24, 1, 1, 1'b1, 1'b1, 1);
        wait_data_kind(SDSPI_DATA_KIND_RESPONSE);
        assert (data.pkt.resp0 == 0) else $fatal(1, "CMD24 response");
        for (int word_idx = 0; word_idx < 128; word_idx++) begin
            expected = 32'hc0000000 | word_idx;
            send_write_word(expected, word_idx == 127);
        end
        wait_data_kind(SDSPI_DATA_KIND_WRITE_DONE);
        for (int word_idx = 0; word_idx < 128; word_idx++) begin
            expected = 32'hc0000000 | word_idx;
            for (int lane = 0; lane < 4; lane++) begin
                assert (u_card.write_data[word_idx * 4 + lane] ==
                    expected[lane * 8 +: 8])
                    else $fatal(1, "write byte %0d", word_idx * 4 + lane);
            end
        end
        assert (u_card.command_crc_ok) else $fatal(1, "command CRC7");
        assert (u_card.write_crc_ok) else $fatal(1, "write data CRC16");

        send_command(18, 0, 1, 1'b1, 1'b0, 2);
        wait_data_kind(SDSPI_DATA_KIND_RESPONSE);
        for (int word_idx = 0; word_idx < 256; word_idx++) begin
            wait_data_kind(SDSPI_DATA_KIND_READ_DATA);
            assert (data.pkt.data == expected_read_word(word_idx % 128))
                else $fatal(1, "multi-read word %0d", word_idx);
            assert (data.pkt.last == (word_idx == 255))
                else $fatal(1, "multi-read last %0d", word_idx);
        end
        assert (!spi_cs_n) else $fatal(1, "CMD18 released CS before CMD12");
        send_command(12, 0, 2, 1'b0, 1'b0, 0);
        wait_data_kind(SDSPI_DATA_KIND_RESPONSE);
        assert (!data.pkt.error && data.pkt.resp0 == 0)
            else $fatal(1, "CMD12 response");

        $display("PASS: sdspi_phy_tb");
        $finish;
    end

    initial begin
        #5000000;
        $fatal(1, "sdspi_phy_tb: timeout");
    end
endmodule
