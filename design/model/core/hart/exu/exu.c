#include "exu.h"
#include "dbg/chk.h"
#include "dbg/log.h"
#include "dbg/vcd.h"

#define EXU_HEARTBEAT_PERIOD 50000000u

static u32 exu_heartbeat_cnt;

static void exu_publish_state(exu_t *exu)
{
    exu->state_o->priv = exu->priv;
    exu->state_o->pc = exu->cur_pc;
    exu->state_o->irq_epc = exu->irq_epc;
    exu->state_o->irq_defer = exu->irq_defer;
    exu->state_o->wfi = exu->wfi;
    exu->state_o->wfi_resume_pc = exu->wfi_resume_pc;
    itf_signal_write_notify(exu->exu_state_out);
}

static void exu_trap_ctrl_cb(void *args)
{
    exu_t *exu = args;
    exu->priv = (rv32g_priv_t)exu->trap_ctrl_i->priv;
    exu->irq_epc = exu->trap_ctrl_i->irq_epc;
    exu->wfi = exu->trap_ctrl_i->wfi;
    exu_publish_state(exu);
}

static void exu_heartbeat(u32 pc, u32 inst, const exu_t *exu)
{
    if (pc < 0x40000000u) {
        return;
    }

    exu_heartbeat_cnt++;
    if ((exu_heartbeat_cnt % EXU_HEARTBEAT_PERIOD) == 0) {
        DBG_LOG(LOG_INFO, "pc heartbeat inst=%u pc=%08x raw=%08x priv=%u ra=%08x sp=%08x a0=%08x a1=%08x\n",
            exu_heartbeat_cnt, pc, inst, (u32)exu->priv, exu->gpr[1],
            exu->gpr[2], exu->gpr[10], exu->gpr[11]);
    }
}

static inline void print_split_line(bool newline)
{
    DBG_LOG(LOG_TRACE, newline ?
        "------------------------------------------------------------------------------\n\n" :
        "------------------------------------------------------------------------------\n"
    );
}

static inline void exu_dump(exu_t *exu, u32 pc)
{
    DBG_LOG(LOG_TRACE, "x0/zero  = %08x   x1/ra = %08x   x2/sp  = %08x   x3/gp  = %08x\n", exu->gpr[0], exu->gpr[1], exu->gpr[2], exu->gpr[3]);
    DBG_LOG(LOG_TRACE, "x4/tp    = %08x   x5/t0 = %08x   x6/t1  = %08x   x7/t2  = %08x\n", exu->gpr[4], exu->gpr[5], exu->gpr[6], exu->gpr[7]);
    DBG_LOG(LOG_TRACE, "x8/s0/fp = %08x   x9/s1 = %08x  x10/a0  = %08x  x11/a1  = %08x\n", exu->gpr[8], exu->gpr[9], exu->gpr[10], exu->gpr[11]);
    DBG_LOG(LOG_TRACE, "x12/a2   = %08x  x13/a3 = %08x  x14/a4  = %08x  x15/a5  = %08x\n", exu->gpr[12], exu->gpr[13], exu->gpr[14], exu->gpr[15]);
    DBG_LOG(LOG_TRACE, "x16/a6   = %08x  x17/a7 = %08x  x18/s2  = %08x  x19/s3  = %08x\n", exu->gpr[16], exu->gpr[17], exu->gpr[18], exu->gpr[19]);
    DBG_LOG(LOG_TRACE, "x20/s4   = %08x  x21/s5 = %08x  x22/s6  = %08x  x23/s7  = %08x\n", exu->gpr[20], exu->gpr[21], exu->gpr[22], exu->gpr[23]);
    DBG_LOG(LOG_TRACE, "x24/s8   = %08x  x25/s9 = %08x  x26/s10 = %08x  x27/s11 = %08x\n", exu->gpr[24], exu->gpr[25], exu->gpr[26], exu->gpr[27]);
    DBG_LOG(LOG_TRACE, "x28/t3   = %08x  x29/t4 = %08x  x30/t5  = %08x  x31/t6  = %08x\n", exu->gpr[28], exu->gpr[29], exu->gpr[30], exu->gpr[31]);
    DBG_LOG(LOG_TRACE, "pc = %08x\n", pc);
}

static void exu_proc_ex_req(exu_t *exu)
{
    if (!itf_fifo_empty(exu->fl_req_slv)) {
        fl_req_if_t fl_req;
        itf_read(exu->fl_req_slv, &fl_req);

        if (!itf_fifo_empty(exu->ex_req_slv)) {
            ex_req_if_t ex_req;
            itf_read(exu->ex_req_slv, &ex_req);
        }
        return;
    }

    if (itf_fifo_empty(exu->ex_req_slv)) {
        return;
    }

    if (exu->wfi) {
        return;
    }

    if (exu->ldst_req_pend) {
        return;
    }

    if (exu->amo_stage != AMO_STAGE_IDLE) {
        return;
    }

    ex_req_if_t ex_req;
    itf_read(exu->ex_req_slv, &ex_req);
    exu_heartbeat(ex_req.pc, ex_req.inst.raw, exu);

    if (!itf_fifo_empty(exu->fl_req_slv)) {
        fl_req_if_t fl_req;
        itf_read(exu->fl_req_slv, &fl_req);
        return;
    }

    if (ex_req.is_boot_code) {
        dbg_disable_log_module(LOG_TRACE);
    }

    typedef void (*ex_req_proc_t)(exu_t *, const ex_req_if_t *);
    static ex_req_proc_t procs[128] = {
        [OPCODE_LUI] = &misc_ex_req_proc,
        [OPCODE_AUIPC] = &misc_ex_req_proc,
        [OPCODE_JAL] = &branch_ex_req_proc,
        [OPCODE_JALR] = &branch_ex_req_proc,
        [OPCODE_BRANCH] = &branch_ex_req_proc,
        [OPCODE_LOAD] = &ldst_ex_req_proc,
        [OPCODE_STORE] = &ldst_ex_req_proc,
        [OPCODE_ALUI] = &alu_ex_req_proc,
        [OPCODE_ALU] = &alu_ex_req_proc,
        [OPCODE_MISC_MEM] = &misc_ex_req_proc,
        [OPCODE_SYSTEM] = &sys_ex_req_proc,
        [OPCODE_AMO] = &amo_ex_req_proc
    };

    ex_req_proc_t proc = procs[ex_req.inst.base.opcode];
    if (proc) {
        print_split_line(false);
        exu_dump(exu, ex_req.pc);
        exu->cur_pc = ex_req.pc;
        exu->irq_epc = ex_req.pc + 4;
        proc(exu, &ex_req);
        print_split_line(true);
    } else {
        DBG_CHECK(0);
    }

    if (ex_req.is_boot_code) {
        dbg_enable_log_module(LOG_TRACE);
    }
}

static void exu_proc_biu_rsp(exu_t *exu)
{
    if (itf_fifo_empty(exu->ldst_rsp_slv)) {
        return;
    }

    ldst_rsp_if_t ldst_rsp;
    itf_read(exu->ldst_rsp_slv, &ldst_rsp);
    if (!ldst_rsp.ok) {
        exu->ldst_req_pend = false;
        exu->amo_stage = AMO_STAGE_IDLE;
        return;
    }

    if (exu->cur_opcode == OPCODE_LOAD || exu->cur_opcode == OPCODE_STORE) {
        ldst_biu_rsp_proc(exu, &ldst_rsp);
    } else if (exu->cur_opcode == OPCODE_AMO) {
        amo_biu_rsp_proc(exu, &ldst_rsp);
    } else {
        DBG_CHECK(0);
    }
}

void exu_clock(exu_t *exu)
{
    mod_clock(&exu->mod);
    exu_proc_ex_req(exu);
    exu_proc_biu_rsp(exu);
    exu_publish_state(exu);
}

void exu_construct(exu_t *exu, const char *parent, const char *name)
{
    mod_construct(&exu->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    dbg_vcd_add_sig("priv", DBG_SIG_TYPE_REG, 2, &exu->priv);
    dbg_vcd_add_sig("cur_pc", DBG_SIG_TYPE_REG, 32, &exu->cur_pc);
    dbg_vcd_add_sig("irq_epc", DBG_SIG_TYPE_REG, 32, &exu->irq_epc);
    dbg_vcd_add_sig("irq_defer", DBG_SIG_TYPE_REG, 1, &exu->irq_defer);
    dbg_vcd_add_sig("cur_opcode", DBG_SIG_TYPE_REG, 7, &exu->cur_opcode);
    dbg_vcd_add_sig("ldst_req_pend", DBG_SIG_TYPE_REG, 1, &exu->ldst_req_pend);
    dbg_vcd_add_sig("ld_rd", DBG_SIG_TYPE_REG, 5, &exu->ld_rd);
    dbg_vcd_add_sig("ld_funct3", DBG_SIG_TYPE_REG, 3, &exu->ld_funct3);
    dbg_vcd_add_sig("amo_stage", DBG_SIG_TYPE_REG, 2, &exu->amo_stage);
    dbg_vcd_add_sig("amo_rd", DBG_SIG_TYPE_REG, 5, &exu->amo_rd);
    dbg_vcd_add_sig("amo_addr", DBG_SIG_TYPE_REG, 32, &exu->amo_addr);
    dbg_vcd_add_sig("amo_lr_set", DBG_SIG_TYPE_REG, 1, &exu->amo_lr_set);
    dbg_vcd_add_sig("amo_rsvd_addr", DBG_SIG_TYPE_REG, 32, &exu->amo_rsvd_addr);
    dbg_vcd_add_sig("wfi", DBG_SIG_TYPE_REG, 1, &exu->wfi);
    dbg_vcd_add_sig("wfi_resume_pc", DBG_SIG_TYPE_REG, 32, &exu->wfi_resume_pc);

    dbg_vcd_add_sig("gpr_ra", DBG_SIG_TYPE_REG, 32, &exu->gpr[1]);
    dbg_vcd_add_sig("gpr_sp", DBG_SIG_TYPE_REG, 32, &exu->gpr[2]);
    dbg_vcd_add_sig("gpr_s0", DBG_SIG_TYPE_REG, 32, &exu->gpr[8]);
    dbg_vcd_add_sig("gpr_a0", DBG_SIG_TYPE_REG, 32, &exu->gpr[10]);
    dbg_vcd_add_sig("gpr_a1", DBG_SIG_TYPE_REG, 32, &exu->gpr[11]);
    dbg_vcd_add_sig("gpr_a2", DBG_SIG_TYPE_REG, 32, &exu->gpr[12]);
    dbg_vcd_add_sig("gpr_a3", DBG_SIG_TYPE_REG, 32, &exu->gpr[13]);
    dbg_vcd_add_sig("gpr_a4", DBG_SIG_TYPE_REG, 32, &exu->gpr[14]);
    dbg_vcd_add_sig("gpr_a5", DBG_SIG_TYPE_REG, 32, &exu->gpr[15]);
    dbg_vcd_add_sig("gpr_t0", DBG_SIG_TYPE_REG, 32, &exu->gpr[5]);
    dbg_vcd_add_sig("gpr_t1", DBG_SIG_TYPE_REG, 32, &exu->gpr[6]);
    dbg_vcd_add_sig("gpr_t2", DBG_SIG_TYPE_REG, 32, &exu->gpr[7]);

    exu->csr_read_req_o = itf_signal_get_src_and_chk(exu->exu_csr_read_req_out);
    exu->csr_read_rsp_i = itf_signal_get_src_and_chk(exu->csr_exu_read_rsp_in);
    exu->csr_write_req_o = itf_signal_get_src_and_chk(exu->exu_csr_write_req_out);
    exu->csr_write_rsp_i = itf_signal_get_src_and_chk(exu->csr_exu_write_rsp_in);
    exu->state_o = itf_signal_get_src_and_chk(exu->exu_state_out);
    exu->trap_ctrl_i = itf_signal_get_src_and_chk(exu->trap_exu_ctrl_in);
    itf_signal_set_wcb(exu->trap_exu_ctrl_in, &exu_trap_ctrl_cb, exu);
}

void exu_reset(exu_t *exu)
{
    mod_reset(&exu->mod);
    exu->gpr[0] = 0;
    for (int i = 1; i < RV32G_GPR_NUM; i++) {
        exu->gpr[i] = (u32)rand();
    }
    exu->cur_opcode = 0;
    exu->ldst_req_pend = false;
    exu->ld_rd = 0;
    exu->ld_funct3 = 0;
    exu->amo_stage = AMO_STAGE_IDLE;
    exu->amo_rd = 0;
    exu->amo_addr = 0;
    exu->amo_s2.u = 0;
    exu->amo_funct375 = 0;
    exu->amo_lr_set = false;
    exu->amo_rsvd_addr = 0;
    exu->wfi = false;
    exu->wfi_resume_pc = 0;
    exu->irq_defer = false;
    exu->priv = RV32G_PRIV_MACHINE;
    exu->cur_pc = 0;
    exu->irq_epc = 0;
    exu_publish_state(exu);
}

void exu_free(exu_t *exu)
{
    mod_free(&exu->mod);
}
