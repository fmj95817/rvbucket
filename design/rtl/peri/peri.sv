module peri(
    input logic      clk,
    input logic      rst_n,
    bti_req_if_t.slv bti_req_slv,
    bti_rsp_if_t.mst bti_rsp_mst,
    output logic     uart_tx,
    input logic      uart_rx,
    ext_irq_if_t.mst uart_irq_mst
);
    typedef enum logic [1:0] {
        SEL_UART,
        SEL_GPIO,
        SEL_CFG
    } sel_t;

    bti_req_if_t uart_req();
    bti_rsp_if_t uart_rsp();
    bti_req_if_t gpio_req();
    bti_rsp_if_t gpio_rsp();
    bti_req_if_t cfg_req();
    bti_rsp_if_t cfg_rsp();
    logic pending;
    sel_t pending_sel;

    wire req_uart = bti_req_slv.pkt.addr[31:12] == 20'h30000;
    wire req_gpio = bti_req_slv.pkt.addr[31:12] == 20'h30001;
    wire req_hsk = bti_req_slv.vld && bti_req_slv.rdy;
    wire rsp_hsk = bti_rsp_mst.vld && bti_rsp_mst.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
            pending_sel <= SEL_UART;
        end else begin
            if (!pending && req_hsk) begin
                pending <= 1'b1;
                pending_sel <= req_uart ? SEL_UART : (req_gpio ? SEL_GPIO : SEL_CFG);
            end else if (pending && rsp_hsk) begin
                pending <= 1'b0;
            end
        end
    end

    assign uart_req.vld = !pending && bti_req_slv.vld && req_uart;
    assign uart_req.pkt = bti_req_slv.pkt;
    assign gpio_req.vld = !pending && bti_req_slv.vld && req_gpio;
    assign gpio_req.pkt = bti_req_slv.pkt;
    assign cfg_req.vld = !pending && bti_req_slv.vld && !req_uart && !req_gpio;
    assign cfg_req.pkt = bti_req_slv.pkt;
    assign bti_req_slv.rdy = !pending &&
        (req_uart ? uart_req.rdy : (req_gpio ? gpio_req.rdy : cfg_req.rdy));

    assign bti_rsp_mst.vld = pending_sel == SEL_UART ? uart_rsp.vld :
        (pending_sel == SEL_GPIO ? gpio_rsp.vld : cfg_rsp.vld);
    assign bti_rsp_mst.pkt = pending_sel == SEL_UART ? uart_rsp.pkt :
        (pending_sel == SEL_GPIO ? gpio_rsp.pkt : cfg_rsp.pkt);
    assign gpio_rsp.rdy = pending && pending_sel == SEL_GPIO && bti_rsp_mst.rdy;
    assign uart_rsp.rdy = pending && pending_sel == SEL_UART && bti_rsp_mst.rdy;
    assign cfg_rsp.rdy = pending && pending_sel == SEL_CFG && bti_rsp_mst.rdy;

    bti_uart u_uart(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (uart_req),
        .bti_rsp_mst (uart_rsp),
        .tx          (uart_tx),
        .rx          (uart_rx),
        .irq         (uart_irq_mst.pkt.irq)
    );

    gpio u_gpio(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (gpio_req),
        .bti_rsp_mst (gpio_rsp)
    );

    gpio u_cfg_stub(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (cfg_req),
        .bti_rsp_mst (cfg_rsp)
    );
endmodule
