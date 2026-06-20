#ifndef CSR_H
#define CSR_H

#include "base/types.h"
#include "base/itf.h"
#include "spec/core/csr.h"
#include "itf/core_timer_if.h"
#include "itf/core_m_irq_if.h"
#include "itf/core_s_irq_if.h"
#include "itf/exu_csr_write_req_if.h"
#include "itf/csr_exu_write_rsp_if.h"
#include "itf/exu_csr_read_req_if.h"
#include "itf/csr_exu_read_rsp_if.h"
#include "itf/core_swi_pend_if.h"

typedef struct csr {
    const u64 *cycle;
    itf_t *core_s_irq_slv;
    itf_t *core_timer_in;
    itf_t *core_m_irq_in;
    itf_t *core_swi_pend_out;
    itf_t *exu_csr_read_req_in;
    itf_t *csr_exu_read_rsp_out;
    itf_t *exu_csr_write_req_in;
    itf_t *csr_exu_write_rsp_out;

    const core_timer_if_t *core_timer_i;
    const core_m_irq_if_t *core_m_irq_i;
    core_swi_pend_if_t *core_swi_pend_o;
    const exu_csr_read_req_if_t *read_req_i;
    csr_exu_read_rsp_if_t *read_rsp_o;
    const exu_csr_write_req_if_t *write_req_i;
    csr_exu_write_rsp_if_t *write_rsp_o;

    rv32g_csr_t regs;
} csr_t;

extern void csr_construct(csr_t *csr, const char *name);
extern void csr_reset(csr_t *csr);
extern void csr_clock(csr_t *csr);
extern void csr_free(csr_t *csrs);

#endif