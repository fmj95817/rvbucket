module peri(
    input logic         clk,
    input logic         rst_n,
    apb_req_if_t.slv    apb_req_slv,
    apb_rsp_if_t.mst    apb_rsp_mst,
    output logic        uart_tx,
    input logic         uart_rx,
    input logic [23:0]  gpio_in,
    output logic [23:0] gpio_out,
    output logic [23:0] gpio_oe,
    ext_irq_if_t.mst    uart_irq_mst,
    ext_irq_if_t.mst    gpio_irq_mst,
    ext_irq_if_t.mst    gtimer_irq_mst
);
    apb_req_if_t gst_req[3]();
    apb_rsp_if_t gst_rsp[3]();

    apb_demux #(
        .GST_NUM  (3),
        .GST_BASE ('{32'h30000000, 32'h30001000, 32'h30002000}),
        .GST_SIZE ('{32'h00000010, 32'h00000010, 32'h00000010})
    ) u_peri_apb_demux(
        .host_apb_req_slv  (apb_req_slv),
        .host_apb_rsp_mst  (apb_rsp_mst),
        .gst_apb_req_msts  (gst_req),
        .gst_apb_rsp_slvs  (gst_rsp)
    );

    apb_uart u_uart(
        .clk         (clk),
        .rst_n       (rst_n),
        .apb_req_slv (gst_req[0]),
        .apb_rsp_mst (gst_rsp[0]),
        .tx          (uart_tx),
        .rx          (uart_rx),
        .irq         (uart_irq_mst.pkt.irq)
    );

    gpio u_gpio(
        .clk         (clk),
        .rst_n       (rst_n),
        .apb_req_slv (gst_req[1]),
        .apb_rsp_mst (gst_rsp[1]),
        .gpio_in     (gpio_in),
        .gpio_out    (gpio_out),
        .gpio_oe     (gpio_oe),
        .irq_mst     (gpio_irq_mst)
    );

    gtimer u_gtimer(
        .clk         (clk),
        .rst_n       (rst_n),
        .apb_req_slv (gst_req[2]),
        .apb_rsp_mst (gst_rsp[2]),
        .irq_mst     (gtimer_irq_mst)
    );
endmodule
