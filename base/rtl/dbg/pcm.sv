module pcm #(
    parameter int COUNTER_NUM = 1,
    parameter logic [31:0] BASE = 32'h30003000,
    localparam int COUNTER_IDX_W = $clog2(COUNTER_NUM)
)(
    input logic                       clk,
    input logic                       rst_n,
    input logic [COUNTER_NUM-1:0]     inc_en,
    apb_req_if_t.slv                  apb_req_slv,
    apb_rsp_if_t.mst                  apb_rsp_mst
);
    localparam int REG_CLEAR = COUNTER_NUM * 8;

    logic pending;
    logic clear;
    logic [63:0] counter[COUNTER_NUM];
    logic [31:0] rsp_data;
    logic rsp_ok;
    logic [31:0] rsp_data_dec;
    logic rsp_ok_dec;

    wire req_hsk = apb_req_slv.psel && apb_req_slv.penable && !pending;
    wire rsp_hsk = pending;
    wire req_write = apb_req_slv.pkt.pwrite;
    wire [31:0] offset = apb_req_slv.pkt.paddr - BASE;
    wire counter_space = offset < (COUNTER_NUM * 8);
    wire [COUNTER_IDX_W-1:0] counter_idx =
        offset[COUNTER_IDX_W+2:3];
    wire counter_hi = offset[2];

    always_comb begin
        rsp_data_dec = 32'h0;
        rsp_ok_dec = 1'b1;
        clear = 1'b0;

        if (offset == REG_CLEAR) begin
            if (req_write)
                clear = 1'b1;
            else
                rsp_ok_dec = 1'b0;
        end else if (counter_space && !req_write) begin
            rsp_data_dec = counter_hi ?
                counter[counter_idx][63:32] :
                counter[counter_idx][31:0];
        end else begin
            rsp_ok_dec = 1'b0;
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
            rsp_data <= 32'h0;
            rsp_ok <= 1'b1;
            for (int i = 0; i < COUNTER_NUM; i++)
                counter[i] <= 64'h0;
        end else begin
            if (req_hsk) begin
                pending <= 1'b1;
                rsp_data <= rsp_data_dec;
                rsp_ok <= rsp_ok_dec;
            end else if (rsp_hsk) begin
                pending <= 1'b0;
            end

            if (req_hsk && clear) begin
                for (int i = 0; i < COUNTER_NUM; i++)
                    counter[i] <= 64'h0;
            end else begin
                for (int i = 0; i < COUNTER_NUM; i++) begin
                    if (inc_en[i])
                        counter[i] <= counter[i] + 64'd1;
                end
            end
        end
    end

    assign apb_rsp_mst.pready = pending;
    assign apb_rsp_mst.pkt.prdata = rsp_data;
    assign apb_rsp_mst.pkt.pslverr = !rsp_ok;
endmodule
