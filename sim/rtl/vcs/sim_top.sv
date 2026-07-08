import "DPI-C" function int uart_stdin_read();

module sim_top;
    logic clk;
    logic rst_n;

    logic uart_tx;
    logic uart_rx;
    logic [23:0] gpio_in;
    logic [23:0] gpio_out;
    logic [23:0] gpio_oe;
    logic uart_rx_ch_vld;
    logic [7:0] uart_rx_ch;
    logic uart_stdin_ch_vld;
    logic [7:0] uart_stdin_ch;
    logic uart_stdin_tx_done;
    logic uart_stdin_tx_busy;
    int uart_stdin_ret;
    logic fast_load_linux_en = 1'b0;
    logic program_valid = 1'b0;
    string program_path;

    localparam logic [31:0] DDR_BASE = 32'h40000000;
    localparam logic [31:0] BIN_TYPE_LINUX = 32'd1;
    localparam int unsigned BIN_HEADER_WORDS = 10;
    localparam int unsigned STAGING_WORD_AW = 23;
    localparam int unsigned STAGING_WORD_NUM = 32'd1 << STAGING_WORD_AW;

    logic [31:0] staging_mem[0:STAGING_WORD_NUM-1];

    assign gpio_in = 24'b0;

    clk_rst u_clk_rst(
        .clk     (clk),
        .rst_n   (rst_n)
    );

    soc u_soc(
        .clk     (clk),
        .rst_n   (rst_n),
        .uart_tx (uart_tx),
        .uart_rx (uart_rx),
        .gpio_in (gpio_in),
        .gpio_out(gpio_out),
        .gpio_oe (gpio_oe)
    );

    uart_rx u_uart_rx(
        .clk     (clk),
        .rst_n   (rst_n),
        .bc      (16'd9),
        .ch_vld  (uart_rx_ch_vld),
        .ch      (uart_rx_ch),
        .rx      (uart_tx)
    );

    uart_tx u_uart_stdin_tx(
        .clk     (clk),
        .rst_n   (rst_n),
        .bc      (16'd9),
        .ch_vld  (uart_stdin_ch_vld),
        .ch      (uart_stdin_ch),
        .done    (uart_stdin_tx_done),
        .tx      (uart_rx)
    );

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            uart_stdin_ch_vld <= 1'b0;
            uart_stdin_ch <= 8'b0;
            uart_stdin_tx_busy <= 1'b0;
            uart_stdin_ret <= -1;
        end else begin
            uart_stdin_ch_vld <= 1'b0;

            if (uart_stdin_tx_done) begin
                uart_stdin_tx_busy <= 1'b0;
            end

            if (!uart_stdin_tx_busy) begin
                uart_stdin_ret = uart_stdin_read();
                if (uart_stdin_ret >= 0) begin
                    uart_stdin_ch <= uart_stdin_ret[7:0];
                    uart_stdin_ch_vld <= 1'b1;
                    uart_stdin_tx_busy <= 1'b1;
                end
            end
        end
    end

    always @(posedge clk) begin
        if (uart_rx_ch_vld) begin
            if (uart_rx_ch != 8'h10) begin
                $write("%c", uart_rx_ch);
                $fflush();
            end else begin
                $finish;
            end
        end
    end

    function automatic int unsigned align4(
        input int unsigned val
    );
        align4 = (val + 3) & ~32'd3;
    endfunction

    function automatic int unsigned align4_words(
        input int unsigned val
    );
        align4_words = align4(val) >> 2;
    endfunction

    task automatic copy_staging_to_flash(
        input int unsigned word_num
    );
        begin
            for (int unsigned i = 0; i < word_num; i++)
                u_soc.u_flash.u_rom.mem[i] = staging_mem[i];
        end
    endtask

    task automatic copy_staging_to_ddr(
        input int unsigned src_word,
        input int unsigned load_addr,
        input int unsigned size
    );
        int unsigned base_word;
        int unsigned word_num;
        begin
            base_word = (load_addr - DDR_BASE) >> 2;
            word_num = align4_words(size);
            for (int unsigned i = 0; i < word_num; i++)
                u_soc.u_ddr.mem[base_word + i] = staging_mem[src_word + i];
        end
    endtask

    task automatic load_program_image(
        input string path
    );
        int unsigned image_type;
        int unsigned itcm_size;
        int unsigned dtcm_size;
        int unsigned kernel_size;
        int unsigned initrd_size;
        int unsigned dtb_size;
        int unsigned kernel_load;
        int unsigned initrd_load;
        int unsigned dtb_load;
        int unsigned boot_words;
        int unsigned image_words;
        int unsigned payload_word;
        begin
            $readmemh(path, staging_mem);

            image_type = staging_mem[0];
            itcm_size = staging_mem[1];
            dtcm_size = staging_mem[2];
            kernel_size = staging_mem[3];
            initrd_size = staging_mem[4];
            dtb_size = staging_mem[5];
            kernel_load = staging_mem[6];
            initrd_load = staging_mem[7];
            dtb_load = staging_mem[8];

            boot_words = BIN_HEADER_WORDS + align4_words(itcm_size) +
                align4_words(dtcm_size);
            image_words = boot_words;
            if (image_type == BIN_TYPE_LINUX) begin
                image_words += align4_words(kernel_size) +
                    align4_words(initrd_size) + align4_words(dtb_size);
            end

            if (fast_load_linux_en && image_type == BIN_TYPE_LINUX)
                copy_staging_to_flash(boot_words);
            else
                copy_staging_to_flash(image_words);

            if (fast_load_linux_en && image_type == BIN_TYPE_LINUX) begin
                payload_word = boot_words;
                copy_staging_to_ddr(payload_word, kernel_load, kernel_size);
                payload_word += align4_words(kernel_size);
                copy_staging_to_ddr(payload_word, initrd_load, initrd_size);
                payload_word += align4_words(initrd_size);
                copy_staging_to_ddr(payload_word, dtb_load, dtb_size);

                u_soc.u_flash.u_rom.mem[3] = 32'd0;
                u_soc.u_flash.u_rom.mem[4] = 32'd0;
                u_soc.u_flash.u_rom.mem[5] = 32'd0;

                $display("sim_top: preloaded linux payloads to DDR: kernel %0d bytes @ 0x%08x, initrd %0d bytes @ 0x%08x, dtb %0d bytes @ 0x%08x",
                    kernel_size, kernel_load, initrd_size, initrd_load,
                    dtb_size, dtb_load);
            end
        end
    endtask

    always @(negedge rst_n) begin
        fast_load_linux_en = $test$plusargs("fast_load_linux");
        program_valid = $value$plusargs("program=%s", program_path);

        if (program_valid)
            load_program_image(program_path);
    end

    initial begin
`ifdef FSDB
        if ($test$plusargs("fsdb")) begin
            $fsdbDumpfile("sim_top.fsdb");
            $fsdbDumpvars(0, sim_top, "+all");
        end
`endif
    end
endmodule
