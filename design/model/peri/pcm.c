#include "pcm.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/pcm_map.h"
#include "dbg/vcd.h"

static bool pcm_counter_offset(u32 offset, u32 *counter_idx, bool *counter_hi)
{
    if ((offset & 0x3u) != 0) {
        return false;
    }
    if (offset >= DBG_PCM_REG_CLEAR) {
        return false;
    }

    *counter_idx = offset >> 3u;
    *counter_hi = ((offset >> 2u) & 1u) != 0;
    return *counter_idx < DBG_PCM_COUNTER_NUM;
}

void pcm_construct(pcm_t *pcm, const char *parent, const char *name,
    u32 base_addr, u32 size)
{
    mod_construct(&pcm->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    pcm->base_addr = base_addr;
    pcm->size = size;
}

void pcm_reset(pcm_t *pcm)
{
    mod_reset(&pcm->mod);
    dbg_pcm_clear();
}

static void pcm_apb_proc(pcm_t *pcm)
{
    if (itf_fifo_empty(pcm->apb_req_slv)) {
        return;
    }
    if (itf_fifo_full(pcm->apb_rsp_mst)) {
        return;
    }

    apb_req_if_t req;
    itf_read(pcm->apb_req_slv, &req);
    DBG_CHECK(req.paddr >= pcm->base_addr);
    DBG_CHECK(req.paddr < pcm->base_addr + pcm->size);

    u32 offset = req.paddr - pcm->base_addr;
    u32 counter_idx = 0;
    bool counter_hi = false;
    apb_rsp_if_t rsp = { .pslverr = false, .prdata = 0 };

    if (offset == DBG_PCM_REG_CLEAR) {
        if (req.pwrite) {
            dbg_pcm_clear();
        } else {
            rsp.pslverr = true;
        }
    } else if (!req.pwrite &&
        pcm_counter_offset(offset, &counter_idx, &counter_hi)) {
        u64 value = dbg_pcm_read_counter(counter_idx);
        rsp.prdata = counter_hi ? (u32)(value >> 32u) : (u32)value;
    } else {
        rsp.pslverr = true;
    }

    itf_write(pcm->apb_rsp_mst, &rsp);
}

void pcm_clock(pcm_t *pcm)
{
    mod_clock(&pcm->mod);
    pcm_apb_proc(pcm);
}

void pcm_free(pcm_t *pcm)
{
    mod_free(&pcm->mod);
    (void)pcm;
}
