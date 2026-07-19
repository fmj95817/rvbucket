#include "core/hart/mmu.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_STG_FIFO_DEPTH 2u
#define TB_OST_DEPTH 4u

typedef struct mmu_tb {
    mod_t mod;
    u64 cycle_val;

    itf_t va_i_req_itf;
    itf_t va_i_rsp_itf;
    itf_t va_d_req_itf;
    itf_t va_d_rsp_itf;
    itf_t pa_i_req_itf;
    itf_t pa_i_rsp_itf;
    itf_t pa_d_req_itf;
    itf_t pa_d_rsp_itf;
    itf_t ptw_req_itf;
    itf_t ptw_rsp_itf;
    itf_t fch_expt_itf;
    itf_t ldst_expt_itf;
    itf_t tlb_flush_itf;
    itf_t exu_state_sig_itf;
    itf_t csr_mmu_state_sig_itf;
    exu_state_if_t *exu_state;
    csr_mmu_state_if_t *csr_mmu_state;

    mmu_t dut;
    ut_sbd_t sbd;
} mmu_tb_t;

static void tb_construct(mmu_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->mod.cycle = &tb->cycle_val;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BTI_REQ_IF_CONSTRUCT(tb, va_i_req_itf, 2);
    BTI_RSP_IF_CONSTRUCT(tb, va_i_rsp_itf, 2);
    BTI_REQ_IF_CONSTRUCT(tb, va_d_req_itf, 2);
    BTI_RSP_IF_CONSTRUCT(tb, va_d_rsp_itf, 2);
    BTI_REQ_IF_CONSTRUCT(tb, pa_i_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(tb, pa_i_rsp_itf, 2);
    BTI_REQ_IF_CONSTRUCT(tb, pa_d_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(tb, pa_d_rsp_itf, 2);
    BTI_REQ_IF_CONSTRUCT(tb, ptw_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(tb, ptw_rsp_itf, 2);
    HART_EXPT_IF_CONSTRUCT(tb, fch_expt_itf, 2);
    HART_EXPT_IF_CONSTRUCT(tb, ldst_expt_itf, 2);
    TLB_FLUSH_IF_CONSTRUCT(tb, tlb_flush_itf, 1);
    EXU_STATE_SIGNAL_IF_CONSTRUCT(tb, exu_state_sig_itf, true, false);
    CSR_MMU_STATE_SIGNAL_IF_CONSTRUCT(tb, csr_mmu_state_sig_itf, true, false);
    tb->exu_state = itf_signal_get_src_and_chk(&tb->exu_state_sig_itf);
    tb->csr_mmu_state = itf_signal_get_src_and_chk(&tb->csr_mmu_state_sig_itf);

    tb->dut.va_i_bti_req_slv = &tb->va_i_req_itf;
    tb->dut.va_i_bti_rsp_mst = &tb->va_i_rsp_itf;
    tb->dut.va_d_bti_req_slv = &tb->va_d_req_itf;
    tb->dut.va_d_bti_rsp_mst = &tb->va_d_rsp_itf;
    tb->dut.pa_i_bti_req_mst = &tb->pa_i_req_itf;
    tb->dut.pa_i_bti_rsp_slv = &tb->pa_i_rsp_itf;
    tb->dut.pa_d_bti_req_mst = &tb->pa_d_req_itf;
    tb->dut.pa_d_bti_rsp_slv = &tb->pa_d_rsp_itf;
    tb->dut.ptw_bti_req_mst = &tb->ptw_req_itf;
    tb->dut.ptw_bti_rsp_slv = &tb->ptw_rsp_itf;
    tb->dut.mmu_fch_expt_mst = &tb->fch_expt_itf;
    tb->dut.ldst_expt_mst = &tb->ldst_expt_itf;
    tb->dut.tlb_flush_slv = &tb->tlb_flush_itf;
    tb->dut.exu_state_in = &tb->exu_state_sig_itf;
    tb->dut.csr_mmu_state_in = &tb->csr_mmu_state_sig_itf;
    tb->dut.mod.cycle = tb->mod.cycle;

    const mmu_conf_t conf = {
        .i_stg_fifo_depth = TB_STG_FIFO_DEPTH,
        .d_stg_fifo_depth = TB_STG_FIFO_DEPTH,
        .ost_depth = TB_OST_DEPTH
    };
    mmu_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);
    ut_sbd_init(&tb->sbd);
}

static void tb_reset(mmu_tb_t *tb)
{
    itf_reset(&tb->va_i_req_itf);
    itf_reset(&tb->va_i_rsp_itf);
    itf_reset(&tb->va_d_req_itf);
    itf_reset(&tb->va_d_rsp_itf);
    itf_reset(&tb->pa_i_req_itf);
    itf_reset(&tb->pa_i_rsp_itf);
    itf_reset(&tb->pa_d_req_itf);
    itf_reset(&tb->pa_d_rsp_itf);
    itf_reset(&tb->ptw_req_itf);
    itf_reset(&tb->ptw_rsp_itf);
    itf_reset(&tb->fch_expt_itf);
    itf_reset(&tb->ldst_expt_itf);
    itf_reset(&tb->tlb_flush_itf);
    itf_reset(&tb->exu_state_sig_itf);
    itf_reset(&tb->csr_mmu_state_sig_itf);
    mmu_reset(&tb->dut);

    tb->exu_state->priv = RV32G_PRIV_MACHINE;
    tb->exu_state->pc = 0x80000000u;
    tb->csr_mmu_state->satp = 0;
    tb->csr_mmu_state->mstatus = 0;
    dbg_vcd_reset();
}

static void tb_clock(mmu_tb_t *tb)
{
    mmu_clock(&tb->dut);

    itf_dbg_clock(&tb->va_i_req_itf);
    itf_dbg_clock(&tb->va_i_rsp_itf);
    itf_dbg_clock(&tb->va_d_req_itf);
    itf_dbg_clock(&tb->va_d_rsp_itf);
    itf_dbg_clock(&tb->pa_i_req_itf);
    itf_dbg_clock(&tb->pa_i_rsp_itf);
    itf_dbg_clock(&tb->pa_d_req_itf);
    itf_dbg_clock(&tb->pa_d_rsp_itf);
    itf_dbg_clock(&tb->ptw_req_itf);
    itf_dbg_clock(&tb->ptw_rsp_itf);
    itf_dbg_clock(&tb->fch_expt_itf);
    itf_dbg_clock(&tb->ldst_expt_itf);
    itf_dbg_clock(&tb->tlb_flush_itf);
    itf_dbg_clock(&tb->exu_state_sig_itf);
    itf_dbg_clock(&tb->csr_mmu_state_sig_itf);

    tb->cycle_val++;
    dbg_vcd_clock();
}

static void tb_free(mmu_tb_t *tb)
{
    mmu_free(&tb->dut);
    itf_free(&tb->va_i_req_itf);
    itf_free(&tb->va_i_rsp_itf);
    itf_free(&tb->va_d_req_itf);
    itf_free(&tb->va_d_rsp_itf);
    itf_free(&tb->pa_i_req_itf);
    itf_free(&tb->pa_i_rsp_itf);
    itf_free(&tb->pa_d_req_itf);
    itf_free(&tb->pa_d_rsp_itf);
    itf_free(&tb->ptw_req_itf);
    itf_free(&tb->ptw_rsp_itf);
    itf_free(&tb->fch_expt_itf);
    itf_free(&tb->ldst_expt_itf);
    itf_free(&tb->tlb_flush_itf);
    itf_free(&tb->exu_state_sig_itf);
    itf_free(&tb->csr_mmu_state_sig_itf);
    mod_free(&tb->mod);
}

static bool tb_cond_ptw_req(mmu_tb_t *tb)
{
    return !itf_fifo_empty(&tb->ptw_req_itf);
}

static bool tb_cond_pa_d_req(mmu_tb_t *tb)
{
    return !itf_fifo_empty(&tb->pa_d_req_itf);
}

static bool tb_cond_pa_i_req(mmu_tb_t *tb)
{
    return !itf_fifo_empty(&tb->pa_i_req_itf);
}

TEST_CASE(mmu_tb_t, queued_fetch_keeps_accept_privilege)
{
    TEST_BEGIN("Queued Fetch Keeps Acceptance Privilege");
    tb_reset(tb);

    bti_req_if_t blocker = {
        .trans_id = 0x100u,
        .cmd = BTI_REQ_CMD_READ,
        .addr = 0x40000000u,
        .size = BTI_REQ_SIZE_B4,
        .strobe = 0xfu
    };
    itf_write(&tb->pa_i_req_itf, &blocker);

    tb->exu_state->priv = RV32G_PRIV_MACHINE;
    tb->csr_mmu_state->satp = 0x80080000u;
    bti_req_if_t fetch = {
        .trans_id = 0x101u,
        .cmd = BTI_REQ_CMD_READ,
        .addr = 0xc02e7fbcu,
        .size = BTI_REQ_SIZE_B4,
        .strobe = 0xfu
    };
    itf_write(&tb->va_i_req_itf, &fetch);
    tb_clock(tb);

    REQUIRE(tb->dut.va_i_req_fifo.num == 1,
        "blocked fetch is held in the staging FIFO");

    tb->exu_state->priv = RV32G_PRIV_USER;
    itf_read(&tb->pa_i_req_itf, &blocker);
    tb_clock(tb);

    REQUIRE(!itf_fifo_empty(&tb->pa_i_req_itf),
        "fetch accepted in M mode remains a bare access");
    REQUIRE(itf_fifo_empty(&tb->ptw_req_itf),
        "later U-mode state does not start a page-table walk");
    bti_req_if_t issued;
    itf_read(&tb->pa_i_req_itf, &issued);
    REQUIRE(issued.addr == fetch.addr && issued.trans_id == fetch.trans_id,
        "the original fetch is issued unchanged");

    TEST_END();
}

TEST_CASE(mmu_tb_t, queued_supervisor_fetch_survives_user_transition)
{
    TEST_BEGIN("Queued Supervisor Fetch Survives User Transition");
    tb_reset(tb);

    const uint32_t kernel_va = 0xc02e7fbcu;
    const uint32_t kernel_pa = 0x402e7fbcu;
    const uint32_t kernel_superpage_pte = 0x100000cfu;
    tb->exu_state->priv = RV32G_PRIV_SUPERVISOR;
    tb->csr_mmu_state->satp = 0x80080000u;

    bti_req_if_t data_req = {
        .trans_id = 0x200u,
        .cmd = BTI_REQ_CMD_READ,
        .addr = 0x40001000u,
        .size = BTI_REQ_SIZE_B4,
        .strobe = 0xfu
    };
    bti_req_if_t fetch_req = {
        .trans_id = 0x201u,
        .cmd = BTI_REQ_CMD_READ,
        .addr = kernel_va,
        .size = BTI_REQ_SIZE_B4,
        .strobe = 0xfu
    };
    itf_write(&tb->va_d_req_itf, &data_req);
    itf_write(&tb->va_i_req_itf, &fetch_req);
    tb_clock(tb);

    REQUIRE(tb->dut.va_i_req_fifo.num == 1,
        "D-side walk leaves the supervisor fetch staged");
    tb->exu_state->priv = RV32G_PRIV_USER;

    RUN_POLL_UNTIL(tb_cond_ptw_req, UT_TIMEOUT);
    bti_req_if_t ptw_req;
    itf_read(&tb->ptw_req_itf, &ptw_req);
    bti_rsp_if_t ptw_rsp = {
        .trans_id = ptw_req.trans_id,
        .data = kernel_superpage_pte,
        .ok = true
    };
    itf_write(&tb->ptw_rsp_itf, &ptw_rsp);

    RUN_POLL_UNTIL(tb_cond_pa_d_req, UT_TIMEOUT);
    bti_req_if_t pa_d_req;
    itf_read(&tb->pa_d_req_itf, &pa_d_req);
    bti_rsp_if_t pa_d_rsp = {
        .trans_id = pa_d_req.trans_id,
        .data = 0,
        .ok = true
    };
    itf_write(&tb->pa_d_rsp_itf, &pa_d_rsp);
    RUN_CYCLES(2);

    RUN_POLL_UNTIL(tb_cond_ptw_req, UT_TIMEOUT);
    itf_read(&tb->ptw_req_itf, &ptw_req);
    ptw_rsp.trans_id = ptw_req.trans_id;
    itf_write(&tb->ptw_rsp_itf, &ptw_rsp);

    RUN_POLL_UNTIL(tb_cond_pa_i_req, UT_TIMEOUT);
    bti_req_if_t pa_i_req;
    itf_read(&tb->pa_i_req_itf, &pa_i_req);
    REQUIRE(pa_i_req.addr == kernel_pa,
        "staged kernel fetch uses its captured supervisor privilege");
    REQUIRE(itf_fifo_empty(&tb->fch_expt_itf),
        "the later user privilege does not create a stale page fault");

    TEST_END();
}

int main(void)
{
    mmu_tb_t tb;

    tb_construct(&tb, "tb");
    TEST_RUN(queued_fetch_keeps_accept_privilege);
    TEST_RUN(queued_supervisor_fetch_survives_user_transition);
    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);

    return ut_sbd_ret(&tb.sbd);
}
