#include "plic.h"
#include "dbg/vcd.h"

#define PLIC_PRIORITY_BASE      0x000000u
#define PLIC_PRIORITY_SIZE      (PLIC_SOURCE_NUM * 4u)
#define PLIC_PENDING_BASE       0x001000u
#define PLIC_PENDING_SIZE       (PLIC_BITFIELD_WORDS * 4u)
#define PLIC_ENABLE_BASE        0x002000u
#define PLIC_ENABLE_SIZE        (PLIC_BITFIELD_WORDS * 4u)
#define PLIC_CONTEXT_BASE       0x200000u
#define PLIC_CONTEXT_SIZE       0x1000u
#define PLIC_CONTEXT_THRESHOLD  0x000u
#define PLIC_CONTEXT_CLAIM      0x004u
#define PLIC_MAX_PRIORITY       7u

static inline bool plic_addr_in(u32 offset, u32 base, u32 size)
{
    return offset >= base && (offset - base) < size;
}

static inline u32 plic_word_mask(u32 word)
{
    if (word + 1u < PLIC_BITFIELD_WORDS) {
        return 0xffffffffu;
    }

    u32 valid_bits = PLIC_SOURCE_NUM - (word * 32u);
    if (valid_bits >= 32u) {
        return 0xffffffffu;
    }
    return (1u << valid_bits) - 1u;
}

static inline u32 plic_source_bit(u32 source)
{
    return 1u << (source & 31u);
}

static inline void plic_set_bit(u32 *bits, u32 source, bool val)
{
    u32 word = source >> 5u;
    u32 mask = plic_source_bit(source);

    if (val) {
        bits[word] |= mask;
    } else {
        bits[word] &= ~mask;
    }
}

static inline bool plic_get_bit(const u32 *bits, u32 source)
{
    return (bits[source >> 5u] & plic_source_bit(source)) != 0u;
}

static u32 plic_find_best_irq(plic_t *plic)
{
    u32 best_source = 0u;
    u32 best_priority = plic->threshold;

    for (u32 source = 1u; source < PLIC_SOURCE_NUM; source++) {
        if (!plic_get_bit(plic->pending, source)) {
            continue;
        }
        if (plic_get_bit(plic->claimed, source)) {
            continue;
        }
        if (!plic_get_bit(plic->enable, source)) {
            continue;
        }

        u32 priority = plic->priority[source];
        if (priority > best_priority) {
            best_source = source;
            best_priority = priority;
        }
    }

    return best_source;
}

static void plic_update_irq(plic_t *plic)
{
    bool irq = plic_find_best_irq(plic) != 0u;

    if (irq != plic->ext_irq_o->irq) {
        plic->ext_irq_o->irq = irq;
        itf_signal_write_notify(plic->conv_ext_irq_out);
    }
}

static u32 plic_claim(plic_t *plic)
{
    u32 source = plic_find_best_irq(plic);
    if (source != 0u) {
        plic_set_bit(plic->pending, source, false);
        plic_set_bit(plic->claimed, source, true);
    }
    return source;
}

static void plic_complete(plic_t *plic, u32 source)
{
    if (source == 0u || source >= PLIC_SOURCE_NUM) {
        return;
    }
    plic_set_bit(plic->claimed, source, false);
}

static u32 plic_read(plic_t *plic, u32 offset, bool *ok)
{
    *ok = true;

    if (plic_addr_in(offset, PLIC_PRIORITY_BASE, PLIC_PRIORITY_SIZE)) {
        u32 source = (offset - PLIC_PRIORITY_BASE) >> 2u;
        return plic->priority[source];
    }

    if (plic_addr_in(offset, PLIC_PENDING_BASE, PLIC_PENDING_SIZE)) {
        u32 word = (offset - PLIC_PENDING_BASE) >> 2u;
        return plic->pending[word] & plic_word_mask(word) & ~1u;
    }

    if (plic_addr_in(offset, PLIC_ENABLE_BASE, PLIC_ENABLE_SIZE)) {
        u32 word = (offset - PLIC_ENABLE_BASE) >> 2u;
        return plic->enable[word] & plic_word_mask(word) & ~1u;
    }

    if (plic_addr_in(offset, PLIC_CONTEXT_BASE, PLIC_CONTEXT_SIZE)) {
        u32 context_offset = offset - PLIC_CONTEXT_BASE;
        if (context_offset == PLIC_CONTEXT_THRESHOLD) {
            return plic->threshold;
        }
        if (context_offset == PLIC_CONTEXT_CLAIM) {
            u32 source = plic_claim(plic);
            plic_update_irq(plic);
            return source;
        }
    }

    *ok = false;
    return 0u;
}

static void plic_write(plic_t *plic, u32 offset, u32 val, bool *ok)
{
    *ok = true;

    if (plic_addr_in(offset, PLIC_PRIORITY_BASE, PLIC_PRIORITY_SIZE)) {
        u32 source = (offset - PLIC_PRIORITY_BASE) >> 2u;
        if (source != 0u) {
            plic->priority[source] = val & PLIC_MAX_PRIORITY;
        }
        plic_update_irq(plic);
        return;
    }

    if (plic_addr_in(offset, PLIC_PENDING_BASE, PLIC_PENDING_SIZE)) {
        return;
    }

    if (plic_addr_in(offset, PLIC_ENABLE_BASE, PLIC_ENABLE_SIZE)) {
        u32 word = (offset - PLIC_ENABLE_BASE) >> 2u;
        plic->enable[word] = val & plic_word_mask(word) & ~1u;
        plic_update_irq(plic);
        return;
    }

    if (plic_addr_in(offset, PLIC_CONTEXT_BASE, PLIC_CONTEXT_SIZE)) {
        u32 context_offset = offset - PLIC_CONTEXT_BASE;
        if (context_offset == PLIC_CONTEXT_THRESHOLD) {
            plic->threshold = val & PLIC_MAX_PRIORITY;
            plic_update_irq(plic);
            return;
        }
        if (context_offset == PLIC_CONTEXT_CLAIM) {
            plic_complete(plic, val);
            plic_update_irq(plic);
            return;
        }
    }

    *ok = false;
}

static void plic_apb_proc(plic_t *plic)
{
    if (itf_fifo_empty(plic->cfg_apb_req_slv)) {
        return;
    }

    apb_req_if_t req;
    itf_read(plic->cfg_apb_req_slv, &req);

    u32 offset = req.paddr - PLIC_BASE;
    bool ok;
    apb_rsp_if_t rsp;
    rsp.prdata = 0u;

    if (req.pwrite) {
        plic_write(plic, offset, req.pwdata, &ok);
    } else {
        rsp.prdata = plic_read(plic, offset, &ok);
    }
    rsp.pslverr = !ok;

    itf_write(plic->cfg_apb_rsp_mst, &rsp);
}

static void plic_sample_inputs(plic_t *plic)
{
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        if (plic->div_ext_irq_ins[i] == NULL) {
            continue;
        }

        const ext_irq_if_t *src = itf_signal_get_src_and_chk(plic->div_ext_irq_ins[i]);
        u32 source = i + 1u;
        if (src->irq && !plic_get_bit(plic->claimed, source)) {
            plic_set_bit(plic->pending, source, true);
        }
    }
}

void plic_construct(plic_t *plic, const char *name, const plic_conf_t *conf)
{
    (void)conf;
    DBG_VCD_MODULE_SCOPE(name);

    dbg_vcd_add_sig("threshold", DBG_SIG_TYPE_REG, 32, &plic->threshold);
    dbg_vcd_add_sig("pending0", DBG_SIG_TYPE_REG, 32, &plic->pending[0]);
    dbg_vcd_add_sig("enable0", DBG_SIG_TYPE_REG, 32, &plic->enable[0]);
    dbg_vcd_add_sig("claimed0", DBG_SIG_TYPE_REG, 32, &plic->claimed[0]);

    plic->ext_irq_o = itf_signal_get_src_and_chk(plic->conv_ext_irq_out);
}

void plic_reset(plic_t *plic)
{
    for (u32 i = 0; i < PLIC_SOURCE_NUM; i++) {
        plic->priority[i] = 0u;
    }
    for (u32 i = 0; i < PLIC_BITFIELD_WORDS; i++) {
        plic->pending[i] = 0u;
        plic->claimed[i] = 0u;
        plic->enable[i] = 0u;
    }
    plic->threshold = 0u;

    plic->ext_irq_o->irq = false;
    itf_signal_write_notify(plic->conv_ext_irq_out);
}

void plic_clock(plic_t *plic)
{
    plic_apb_proc(plic);
    plic_sample_inputs(plic);
    plic_update_irq(plic);
}

void plic_free(plic_t *plic)
{
    (void)plic;
}
