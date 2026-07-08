#include "exu.h"
#include "utils.h"
#include "dbg/chk.h"
#include "dbg/log.h"

#define DECL_MISC_HANDLER(inst) static void inst##_ex_req_handler(exu_t *exu, const ex_req_if_t *req)
#define CALL_MISC_HANDLER(inst, exu, req) inst##_ex_req_handler(exu, req)

DECL_MISC_HANDLER(lui)
{
    u32 rd = req->inst.u.rd;
    u32 imm = u_imm_decode(&req->inst);
    set_gpr(exu, rd, imm);

    DBG_LOG(LOG_TRACE, "lui %s, 0x%08x\n", gpr_name(rd), imm);
}

DECL_MISC_HANDLER(auipc)
{
    u32 rd = req->inst.u.rd;
    u32 imm = u_imm_decode(&req->inst);
    set_gpr(exu, rd, req->pc + imm);

    DBG_LOG(LOG_TRACE, "auipc %s, 0x%08x\n", gpr_name(rd), imm);
}

DECL_MISC_HANDLER(fence)
{
    u32 fm = req->inst.i.imm_11_0 >> 8;
    if (fm == 0b0000) {
        DBG_LOG(LOG_TRACE, "fence\n");
    } else if (fm == 0b1000) {
        DBG_LOG(LOG_TRACE, "fence.tso\n");
    } else {
        DBG_CHECK(0);
    }
}

DECL_MISC_HANDLER(fence_i)
{
    DBG_LOG(LOG_TRACE, "fence.i\n");
    DBG_CHECK(!itf_fifo_full(exu->l1i_flush_mst));
    l1_flush_if_t pkt = {};
    itf_write(exu->l1i_flush_mst, &pkt);
}

DECL_MISC_HANDLER(cbo)
{
    DBG_CHECK(!itf_fifo_full(exu->ldst_req_mst));

    ldst_req_cmo_t cmo;
    const char *name;
    switch (req->inst.i.imm_11_0) {
    case 0:
        cmo = LDST_REQ_CMO_INVAL;
        name = "cbo.inval";
        break;
    case 1:
        cmo = LDST_REQ_CMO_CLEAN;
        name = "cbo.clean";
        break;
    case 2:
        cmo = LDST_REQ_CMO_FLUSH;
        name = "cbo.flush";
        break;
    default:
        DBG_CHECK(0);
        return;
    }

    u32 rs1 = req->inst.i.rs1;
    ldst_req_if_t ldst_req = {
        .addr = get_gpr(exu, rs1),
        .st = false,
        .cmo = cmo,
        .size = LDST_REQ_SIZE_B4,
        .data = 0,
        .strobe = 0
    };
    itf_write(exu->ldst_req_mst, &ldst_req);
    exu->cur_opcode = OPCODE_MISC_MEM;
    exu->ldst_req_pend = true;
    exu->irq_defer = true;

    DBG_LOG(LOG_TRACE, "%s (%s) # 0x%08x\n",
        name, gpr_name(rs1), ldst_req.addr);
}

void misc_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    u32 opcode = ex_req->inst.base.opcode;
    if (opcode == OPCODE_LUI) {
        CALL_MISC_HANDLER(lui, exu, ex_req);
    } else if (opcode == OPCODE_AUIPC) {
        CALL_MISC_HANDLER(auipc, exu, ex_req);
    } else if (opcode == OPCODE_MISC_MEM) {
        u32 funct3 = ex_req->inst.i.funct3;
        if (funct3 == MISC_MEM_FUNCT3_FENCE) {
            CALL_MISC_HANDLER(fence, exu, ex_req);
        } else if (funct3 == MISC_MEM_FUNCT3_FENCE_I) {
            CALL_MISC_HANDLER(fence_i, exu, ex_req);
        } else if (funct3 == MISC_MEM_FUNCT3_CBO) {
            CALL_MISC_HANDLER(cbo, exu, ex_req);
        } else {
            DBG_CHECK(0);
        }
    } else {
        DBG_CHECK(0);
    }
}
