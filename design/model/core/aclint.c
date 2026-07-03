#include "aclint.h"
#include "base/def.h"
#include "dbg/vcd.h"
#include "dbg/chk.h"

#define MTIME              0u
#define MTIMEH             4u
#define MTIMECMP(hart_id)  ((hart_id) * 8u + 0u)
#define MTIMECMPH(hart_id) ((hart_id) * 8u + 4u)
#define MSIP(hart_id)      ((hart_id) * 4u)
#define SETSSIP(hart_id)   ((hart_id) * 4u)

void aclint_construct(aclint_t *aclint, const char *parent, const char *name, const aclint_conf_t *conf)
{
    mod_construct(&aclint->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    aclint->conf = *conf;
    dbg_vcd_add_sig("mtime", DBG_SIG_TYPE_REG, 64, &aclint->mtime.raw);
    dbg_vcd_add_sig("mtimecmp", DBG_SIG_TYPE_REG, 64, &aclint->mtimecmp[0].raw);

    aclint->core_timer_o = itf_signal_get_src_and_chk(aclint->core_timer_out);
    for (u32 i = 0; i < HART_NUM; i++) {
        aclint->core_m_irq_o[i] = itf_signal_get_src_and_chk(aclint->core_m_irq_outs[i]);
        aclint->core_swi_pend_i[i] = itf_signal_get_src_and_chk(aclint->core_swi_pend_ins[i]);
    }
}

void aclint_reset(aclint_t *aclint)
{
    mod_reset(&aclint->mod);
    aclint->mtime.raw = 0ull;
    aclint->mtime_cycle_cnt = 0u;
    for (u32 i = 0; i < HART_NUM; i++) {
        aclint->mtimecmp[i].raw = 0xffffffffffffffffull;
        aclint->mtime_exceed_old[i] = false;
        aclint->core_m_irq_o[i]->msw = false;
        aclint->core_m_irq_o[i]->mtimer = false;
        itf_signal_write_notify(aclint->core_m_irq_outs[i]);
    }
}

static void timer_proc(aclint_t *aclint)
{
    DBG_CHECK(aclint->core_timer_out != NULL);

    aclint->mtime_cycle_cnt++;
    if (aclint->mtime_cycle_cnt == aclint->conf.mtimer_tick_cycles) {
        aclint->mtime_cycle_cnt = 0u;
        aclint->mtime.raw++;

        aclint->core_timer_o->time = aclint->mtime.raw;
        itf_signal_write_notify(aclint->core_timer_out);
    }
}

static inline u32 gen_write_mask(u8 strb)
{
    u32 mask = 0u;
    for (u32 i = 0; i < 4u; i++) {
        if (strb & (1u << i)) {
            mask |= 0xffu << (i * 8u);
        }
    }
    return mask;
}

static inline void reg32_rw(u32 *reg, u32 *val, bool write, u8 strb)
{
    if (write) {
        u32 mask = gen_write_mask(strb);
        u32 old = *reg;
        *reg = old & (~mask);
        *reg |= (*val & mask);
    } else {
        *val = *reg;
    }
}

static bool mtime_reg_rw(aclint_t *aclint, u32 addr, u32 *val, bool write, u8 strb)
{
    bool success = false;
    switch (addr) {
        case MTIME: {
            reg32_rw(&aclint->mtime.reg.lo, val, write, strb);
            success = true;
            break;
        }
        case MTIMEH: {
            reg32_rw(&aclint->mtime.reg.hi, val, write, strb);
            success = true;
            break;
        }
    }
    return success;
}

static bool mtimecmp_reg_rw(aclint_t *aclint, u32 addr, u32 *val, bool write, u8 strb)
{
    for (u32 i = 0; i < HART_NUM; i++) {
        if (addr == MTIMECMP(i)) {
            reg32_rw(&aclint->mtimecmp[i].reg.lo, val, write, strb);
            return true;
        } else if (addr == MTIMECMPH(i)) {
            reg32_rw(&aclint->mtimecmp[i].reg.hi, val, write, strb);
            return true;
        }
    }
    return false;
}

static bool mswi_reg_rw(aclint_t *aclint, u32 addr, u32 *val, bool write, u8 strb)
{
    for (u32 i = 0; i < HART_NUM; i++) {
        if (addr != MSIP(i)) {
            continue;
        }
        if (write && (strb & 1u)) {
            aclint->core_m_irq_o[i]->msw = ((*val & 1u) != 0);
            itf_signal_write_notify(aclint->core_m_irq_outs[i]);
            return true;
        } else if (!write) {
            *val = aclint->core_swi_pend_i[i]->msip ? 1u : 0u;
            return true;
        }
    }
    return false;
}

static bool sswi_reg_rw(aclint_t *aclint, u32 addr, u32 *val, bool write, u8 strb)
{
    for (u32 i = 0; i < HART_NUM; i++) {
        if ((addr == SETSSIP(i)) && write && (strb & 1u) && (*val & 1u)) {
            DBG_CHECK(aclint->core_s_irq_msts[i] != NULL);
            DBG_CHECK(!itf_fifo_full(aclint->core_s_irq_msts[i]));
            core_s_irq_if_t core_s_irq;
            core_s_irq.ssw = true;
            itf_write(aclint->core_s_irq_msts[i], &core_s_irq);
            return true;
        }
    }
    return false;
}

static bool aclint_reg_rw(aclint_t *aclint, u32 addr, u32 *val, bool write, u8 strb)
{
    if (ADDR_IN(addr, aclint->conf.mtimer_base, aclint->conf.mtimer_size)) {
        return mtime_reg_rw(aclint, addr - aclint->conf.mtimer_base, val, write, strb);
    }

    if (ADDR_IN(addr, aclint->conf.mtimecmp_base, aclint->conf.mtimecmp_size)) {
        return mtimecmp_reg_rw(aclint, addr - aclint->conf.mtimecmp_base, val, write, strb);
    }

    if (ADDR_IN(addr, aclint->conf.mswi_base, aclint->conf.mswi_size)) {
        return mswi_reg_rw(aclint, addr - aclint->conf.mswi_base, val, write, strb);
    }

    if (ADDR_IN(addr, aclint->conf.sswi_base, aclint->conf.sswi_size)) {
        return sswi_reg_rw(aclint, addr - aclint->conf.sswi_base, val, write, strb);
    }

    return false;
}

static void apb_cfg_proc(aclint_t *aclint)
{
    DBG_CHECK(aclint->cfg_apb_req_slv);
    DBG_CHECK(aclint->cfg_apb_rsp_mst);

    if (itf_fifo_empty(aclint->cfg_apb_req_slv)) {
        return;
    }

    if (itf_fifo_full(aclint->cfg_apb_rsp_mst)) {
        return;
    }

    apb_req_if_t req;
    itf_read(aclint->cfg_apb_req_slv, &req);

    apb_rsp_if_t rsp;
    u32 *rw_val = req.pwrite ? &req.pwdata : &rsp.prdata;
    rsp.pslverr = !aclint_reg_rw(aclint, req.paddr, rw_val, req.pwrite, req.pstrb);

    itf_write(aclint->cfg_apb_rsp_mst, &rsp);
}

static void raise_or_clear_mtimer_irq(aclint_t *aclint, u32 hart_id)
{
    DBG_CHECK(hart_id < HART_NUM);

    bool m_exceed = (aclint->mtime.raw >= aclint->mtimecmp[hart_id].raw);
    if (m_exceed != aclint->mtime_exceed_old[hart_id]) {
        aclint->core_m_irq_o[hart_id]->mtimer = m_exceed;
        itf_signal_write_notify(aclint->core_m_irq_outs[hart_id]);
        aclint->mtime_exceed_old[hart_id] = m_exceed;
    }
}

static void core_irq_proc(aclint_t *aclint)
{
    for (u32 i = 0; i < HART_NUM; i++) {
        raise_or_clear_mtimer_irq(aclint, i);
    }
}

void aclint_clock(aclint_t *aclint)
{
    mod_clock(&aclint->mod);
    timer_proc(aclint);
    apb_cfg_proc(aclint);
    core_irq_proc(aclint);
}

void aclint_free(aclint_t *aclint)
{
    mod_free(&aclint->mod);
}
