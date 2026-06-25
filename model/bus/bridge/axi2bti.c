#include "axi2bti.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

static u32 axi2bti_next_addr(u32 addr, u8 size, u8 burst)
{
    switch (burst) {
    case 0: /* FIXED */
        return addr;
    case 1: /* INCR */
        return addr + (1u << size);
    default: /* WRAP not supported */
        return addr + (1u << size);
    }
}

void axi2bti_construct(axi2bti_t *br, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(br->axi4_ar_slv);
    DBG_CHECK(br->axi4_r_mst);
    DBG_CHECK(br->axi4_aw_slv);
    DBG_CHECK(br->axi4_w_slv);
    DBG_CHECK(br->axi4_b_mst);
    DBG_CHECK(br->bti_req_mst);
    DBG_CHECK(br->bti_rsp_slv);
}

void axi2bti_reset(axi2bti_t *br)
{
    br->state = AXI2BTI_STATE_IDLE;
    br->beat_idx = 0;
    br->bti_rsp_pending = false;
    br->next_bti_trans_id = 1;
}

static void axi2bti_proc_idle(axi2bti_t *br)
{
    if (br->state != AXI2BTI_STATE_IDLE) {
        return;
    }

    /* check AR (read) first */
    if (!itf_fifo_empty(br->axi4_ar_slv)) {
        if (itf_fifo_full(br->bti_req_mst)) {
            return;
        }

        axi4_ar_if_t ar;
        itf_fifo_get_front(br->axi4_ar_slv, &ar);

        if (ar.burst == AXI4_AR_BURST_WRAP) {
            itf_fifo_pop_front(br->axi4_ar_slv);
            if (!itf_fifo_full(br->axi4_r_mst)) {
                axi4_r_if_t r = {
                    .id = ar.id, .data = 0, .resp = AXI4_R_RESP_SLVERR, .last = true
                };
                itf_write(br->axi4_r_mst, &r);
            }
            return;
        }

        itf_fifo_pop_front(br->axi4_ar_slv);

        br->axid = ar.id;
        br->axaddr = ar.addr;
        br->axlen = ar.len;
        br->axsize = ar.size;
        br->axburst = ar.burst;
        br->beat_idx = 0;
        br->bti_rsp_pending = true;

        bti_req_if_t req = {
            .trans_id = br->next_bti_trans_id++,
            .cmd = BTI_REQ_CMD_READ,
            .addr = ar.addr,
            .data = 0,
            .strobe = 0
        };
        itf_write(br->bti_req_mst, &req);
        br->state = AXI2BTI_STATE_RD_BURST;
        return;
    }

    /* check AW (write) */
    if (!itf_fifo_empty(br->axi4_aw_slv)) {
        axi4_aw_if_t aw;
        itf_fifo_get_front(br->axi4_aw_slv, &aw);

        if (aw.burst == AXI4_AW_BURST_WRAP) {
            itf_fifo_pop_front(br->axi4_aw_slv);
            if (!itf_fifo_full(br->axi4_b_mst)) {
                axi4_b_if_t b = { .id = aw.id, .resp = AXI4_B_RESP_SLVERR };
                itf_write(br->axi4_b_mst, &b);
            }
            return;
        }

        itf_fifo_pop_front(br->axi4_aw_slv);

        br->axid = aw.id;
        br->axaddr = aw.addr;
        br->axlen = aw.len;
        br->axsize = aw.size;
        br->axburst = aw.burst;
        br->beat_idx = 0;
        br->bti_rsp_pending = false;
        br->state = AXI2BTI_STATE_WR_BURST;
        return;
    }
}

static void axi2bti_proc_rd_burst(axi2bti_t *br)
{
    if (br->state != AXI2BTI_STATE_RD_BURST) {
        return;
    }

    if (br->bti_rsp_pending && !itf_fifo_empty(br->bti_rsp_slv)) {
        if (itf_fifo_full(br->axi4_r_mst)) {
            return;
        }

        bti_rsp_if_t rsp;
        itf_read(br->bti_rsp_slv, &rsp);
        br->bti_rsp_pending = false;

        bool ok = rsp.ok;
        bool last = (br->beat_idx == br->axlen) || !ok;

        axi4_r_if_t r = {
            .id = br->axid,
            .data = ok ? rsp.data : 0,
            .resp = ok ? 0 : 2,
            .last = last
        };
        itf_write(br->axi4_r_mst, &r);

        if (last) {
            br->state = AXI2BTI_STATE_IDLE;
            return;
        }

        br->beat_idx++;
    }

    if (!br->bti_rsp_pending && br->beat_idx <= br->axlen) {
        if (itf_fifo_full(br->bti_req_mst)) {
            return;
        }

        u32 addr = axi2bti_next_addr(br->axaddr, br->axsize, br->axburst);
        br->axaddr = addr;

        bti_req_if_t req = {
            .trans_id = br->next_bti_trans_id++,
            .cmd = BTI_REQ_CMD_READ,
            .addr = addr,
            .data = 0,
            .strobe = 0
        };
        itf_write(br->bti_req_mst, &req);
        br->bti_rsp_pending = true;
    }
}

static void axi2bti_proc_wr_burst(axi2bti_t *br)
{
    if (br->state != AXI2BTI_STATE_WR_BURST) {
        return;
    }

    if (br->bti_rsp_pending && !itf_fifo_empty(br->bti_rsp_slv)) {
        bti_rsp_if_t rsp;
        itf_read(br->bti_rsp_slv, &rsp);
        br->bti_rsp_pending = false;

        bool all_beats_done = (br->beat_idx > br->axlen);

        if (!rsp.ok || all_beats_done) {
            if (!itf_fifo_full(br->axi4_b_mst)) {
                axi4_b_if_t b = {
                    .id = br->axid,
                    .resp = rsp.ok ? 0 : 2
                };
                itf_write(br->axi4_b_mst, &b);
                br->state = AXI2BTI_STATE_IDLE;
            }
            return;
        }
    }

    if (!br->bti_rsp_pending && br->beat_idx <= br->axlen) {
        if (itf_fifo_empty(br->axi4_w_slv)) {
            return;
        }
        if (itf_fifo_full(br->bti_req_mst)) {
            return;
        }

        axi4_w_if_t w;
        itf_read(br->axi4_w_slv, &w);

        u32 addr = br->axaddr;

        bti_req_if_t req = {
            .trans_id = br->next_bti_trans_id++,
            .cmd = BTI_REQ_CMD_WRITE,
            .addr = addr,
            .data = w.data,
            .strobe = w.strb
        };
        itf_write(br->bti_req_mst, &req);
        br->bti_rsp_pending = true;

        if (w.last) {
            br->beat_idx = br->axlen + 1;
        } else {
            br->axaddr = axi2bti_next_addr(addr, br->axsize, br->axburst);
            br->beat_idx++;
        }
    }
}

void axi2bti_clock(axi2bti_t *br)
{
    axi2bti_proc_idle(br);
    axi2bti_proc_rd_burst(br);
    axi2bti_proc_wr_burst(br);
}

void axi2bti_free(axi2bti_t *br) {}
