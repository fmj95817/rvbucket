#include <stdio.h>
#include <stdlib.h>
#include "core/hart/bpu.h"
#include "spec/core/isa.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

static u32 tb_encode_jal(u32 rd, s32 imm)
{
    u32 uimm = (u32)imm;

    return (((uimm >> 20) & 0x1) << 31) |
           (((uimm >> 1) & 0x3ff) << 21) |
           (((uimm >> 11) & 0x1) << 20) |
           (((uimm >> 12) & 0xff) << 12) |
           ((rd & 0x1f) << 7) |
           OPCODE_JAL;
}

static u32 tb_encode_jalr(u32 rd, u32 rs1, s32 imm)
{
    return (((u32)imm & 0xfff) << 20) |
           ((rs1 & 0x1f) << 15) |
           ((rd & 0x1f) << 7) |
           OPCODE_JALR;
}

static u32 tb_cond_bht_idx(u32 pc)
{
    return (pc >> 2) & (BPU_COND_BHT_SIZE - 1u);
}

typedef struct bpu_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t pred_req_sig_itf;
    itf_t pred_rsp_sig_itf;
    itf_t update_sig_itf;

    bpu_t dut;
    ut_sbd_t sbd;
} bpu_tb_t;

static void tb_construct(bpu_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BPU_PRED_REQ_SIGNAL_IF_CONSTRUCT(tb, pred_req_sig_itf, false, false);
    BPU_PRED_RSP_SIGNAL_IF_CONSTRUCT(tb, pred_rsp_sig_itf, false, false);
    BPU_UPDATE_SIGNAL_IF_CONSTRUCT(tb, update_sig_itf, false, false);

    tb->dut.pred_req_slv = &tb->pred_req_sig_itf;
    tb->dut.pred_rsp_mst = &tb->pred_rsp_sig_itf;
    tb->dut.update_slv = &tb->update_sig_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    bpu_construct(&tb->dut, tb->mod.hier_name, "u_dut");

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(bpu_tb_t *tb)
{
    itf_reset(&tb->pred_req_sig_itf);
    itf_reset(&tb->pred_rsp_sig_itf);
    itf_reset(&tb->update_sig_itf);
    bpu_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_free(bpu_tb_t *tb)
{
    bpu_free(&tb->dut);
    itf_free(&tb->pred_req_sig_itf);
    itf_free(&tb->pred_rsp_sig_itf);
    itf_free(&tb->update_sig_itf);
}

static void tb_clock(bpu_tb_t *tb)
{
    bpu_clock(&tb->dut);

    itf_dbg_clock(&tb->pred_req_sig_itf);
    itf_dbg_clock(&tb->pred_rsp_sig_itf);
    itf_dbg_clock(&tb->update_sig_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static bpu_pred_rsp_if_t tb_predict(bpu_tb_t *tb, u32 pc, u32 ir)
{
    bpu_pred_req_if_t req = {
        .vld = true,
        .pc = pc,
        .ir = ir
    };
    itf_write(&tb->pred_req_sig_itf, &req);

    bpu_pred_rsp_if_t rsp;
    itf_read(&tb->pred_rsp_sig_itf, &rsp);
    return rsp;
}

static void tb_update(bpu_tb_t *tb, const bpu_update_if_t *update)
{
    itf_write(&tb->update_sig_itf, update);
    tb_clock(tb);

    bpu_update_if_t clear = {};
    itf_write(&tb->update_sig_itf, &clear);
}

TEST_CASE(bpu_tb_t, nop_default_prediction)
{
    TEST_BEGIN("NOP Default Prediction");

    tb_reset(tb);
    bpu_pred_rsp_if_t rsp = tb_predict(tb, 0x80000000, 0x00000013);
    REQUIRE(rsp.vld, "response is valid");
    REQUIRE(!rsp.is_ctrl, "NOP is not control");
    REQUIRE(!rsp.pred_taken, "NOP is not predicted taken");
    REQUIRE(rsp.pred_pc == 0x80000004, "NOP pred_pc is pc+4");

    TEST_END();
}

TEST_CASE(bpu_tb_t, backward_branch_default_taken)
{
    TEST_BEGIN("Backward Branch Default Taken");

    tb_reset(tb);
    const u32 branch_pc = 0x80000008;
    const u32 branch_ir = 0xfe000ee3;
    rv32g_inst_t branch = { .raw = branch_ir };
    const u32 target = branch_pc + b_imm_decode(&branch).u;

    bpu_pred_rsp_if_t rsp = tb_predict(tb, branch_pc, branch_ir);
    REQUIRE(rsp.is_ctrl, "branch is control");
    REQUIRE(rsp.pred_taken, "untrained backward branch predicts taken");
    REQUIRE(rsp.pred_pc == target, "branch target is decoded");

    TEST_END();
}

TEST_CASE(bpu_tb_t, cond_bht_train_and_hit)
{
    TEST_BEGIN("Cond BHT Train and Hit");

    tb_reset(tb);
    const u32 branch_pc = 0x80000000;
    const u32 branch_ir = 0x00000463;
    const u32 idx = tb_cond_bht_idx(branch_pc);

    bpu_update_if_t update = {
        .vld = true,
        .pc = branch_pc,
        .ir = branch_ir,
        .taken = true,
        .target_pc = branch_pc + 8,
        .pred_true = false,
        .is_boot_code = false
    };
    tb_update(tb, &update);
    REQUIRE(tb->dut.cond_bht[idx].vld, "BHT entry allocated");
    REQUIRE(tb->dut.cond_bht[idx].counter == 2, "taken trains weak taken");

    bpu_pred_rsp_if_t rsp = tb_predict(tb, branch_pc, branch_ir);
    REQUIRE(rsp.pred_taken, "trained BHT predicts taken");
    REQUIRE(rsp.cond_bht_hit, "prediction reports BHT hit");
    REQUIRE(*tb->dut.perf.cond_bht_hit == 0, "BHT hit counter waits for update");

    update.taken = false;
    update.target_pc = branch_pc + 4;
    update.pred_cond_bht_hit = rsp.cond_bht_hit;
    tb_update(tb, &update);
    REQUIRE(*tb->dut.perf.cond_bht_hit == 1, "BHT hit counter increments");
    REQUIRE(tb->dut.cond_bht[idx].counter == 1, "not-taken decrements counter");

    TEST_END();
}

TEST_CASE(bpu_tb_t, ras_call_return)
{
    TEST_BEGIN("RAS Call Return");

    tb_reset(tb);
    const u32 call_pc = 0x80000000;
    const u32 call_target = 0x80000100;
    const u32 ret_pc = call_pc + 4;
    const u32 jal = tb_encode_jal(1, call_target - call_pc);
    const u32 ret = tb_encode_jalr(0, 1, 0);

    bpu_update_if_t call_update = {
        .vld = true,
        .pc = call_pc,
        .ir = jal,
        .taken = true,
        .target_pc = call_target,
        .pred_true = true,
        .is_boot_code = false
    };
    tb_update(tb, &call_update);
    REQUIRE(tb->dut.ras.count == 1, "JAL link pushes RAS");

    bpu_pred_rsp_if_t rsp = tb_predict(tb, call_target, ret);
    REQUIRE(rsp.pred_taken, "return predicts taken");
    REQUIRE(rsp.pred_pc == ret_pc, "return target comes from RAS");
    REQUIRE(rsp.jalr_ras_hit, "prediction reports RAS hit");
    REQUIRE(*tb->dut.perf.ras_pred == 0, "RAS pred counter waits for update");

    bpu_update_if_t ret_update = {
        .vld = true,
        .pc = call_target,
        .ir = ret,
        .taken = true,
        .target_pc = ret_pc,
        .pred_true = true,
        .is_boot_code = false,
        .pred_jalr_ras_hit = rsp.jalr_ras_hit
    };
    tb_update(tb, &ret_update);
    REQUIRE(*tb->dut.perf.ras_pred == 1, "RAS pred counter increments");
    REQUIRE(tb->dut.ras.count == 0, "return pops RAS");

    TEST_END();
}

TEST_CASE(bpu_tb_t, jalr_btb_train_and_hit)
{
    TEST_BEGIN("JALR BTB Train and Hit");

    tb_reset(tb);
    const u32 jalr_pc = 0x80000000;
    const u32 jalr_target = 0x80000200;
    const u32 jalr = tb_encode_jalr(0, 2, 0);

    bpu_pred_rsp_if_t miss = tb_predict(tb, jalr_pc, jalr);
    REQUIRE(!miss.pred_taken, "untrained JALR does not predict taken");
    REQUIRE(miss.jalr_btb_miss, "prediction reports JALR BTB miss");
    REQUIRE(*tb->dut.perf.jalr_btb_miss == 0, "JALR BTB miss counter waits for update");

    bpu_update_if_t update = {
        .vld = true,
        .pc = jalr_pc,
        .ir = jalr,
        .taken = true,
        .target_pc = jalr_target,
        .pred_true = false,
        .is_boot_code = false,
        .pred_jalr_btb_miss = miss.jalr_btb_miss
    };
    tb_update(tb, &update);
    REQUIRE(*tb->dut.perf.jalr_btb_miss == 1, "JALR BTB miss counted");

    bpu_pred_rsp_if_t hit = tb_predict(tb, jalr_pc, jalr);
    REQUIRE(hit.pred_taken, "trained JALR predicts taken");
    REQUIRE(hit.pred_pc == jalr_target, "trained JALR target is used");
    REQUIRE(hit.jalr_btb_hit, "prediction reports JALR BTB hit");
    update.pred_jalr_btb_miss = false;
    update.pred_jalr_btb_hit = hit.jalr_btb_hit;
    tb_update(tb, &update);
    REQUIRE(*tb->dut.perf.jalr_btb_hit == 1, "JALR BTB hit counted");

    TEST_END();
}

int main(void)
{
    bpu_tb_t tb;

    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(nop_default_prediction);
    TEST_RUN(backward_branch_default_taken);
    TEST_RUN(cond_bht_train_and_hit);
    TEST_RUN(ras_call_return);
    TEST_RUN(jalr_btb_train_and_hit);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);

    return ut_sbd_ret(&tb.sbd);
}
