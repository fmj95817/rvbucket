#include "exu.h"
#include "utils.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/log.h"

#define DECL_AMO_EX_REQ_HANDLER(inst) static bool inst##_ex_req_handler( \
    exu_t *exu, const ex_req_if_t *ex_req, ldst_req_if_t *ldst_req, bool biu_rdy)
#define DECL_AMO_BIU_RSP_HANDLER(inst) static bool inst##_biu_rsp_handler( \
    exu_t *exu, const ldst_rsp_if_t *ldst_rsp, ldst_req_if_t *st_req, bool biu_rdy)

#define GET_AMO_EX_REQ_HANDLER(inst) &inst##_ex_req_handler
#define GET_AMO_BIU_RSP_HANDLER(inst) &inst##_biu_rsp_handler

#define CALL_AMO_EX_REQ_HANDLER(inst, exu, ex_req, ldst_req, biu_rdy) \
    inst##_ex_req_handler(exu, ex_req, ldst_req, biu_rdy)
#define CALL_AMO_BIU_RSP_HANDLER(inst, exu, ldst_rsp, st_req, biu_rdy) \
    inst##_biu_rsp_handler(exu, ldst_rsp, st_req, biu_rdy)

typedef bool (*amo_ex_req_handler_t)( \
    exu_t *exu, const ex_req_if_t *ex_req, ldst_req_if_t *ldst_req, bool biu_rdy);
typedef bool (*amo_biu_rsp_handler_t)( \
    exu_t *exu, const ldst_rsp_if_t *ldst_rsp, ldst_req_if_t *st_req, bool biu_rdy);

DECL_AMO_EX_REQ_HANDLER(lr)
{
    if (!biu_rdy) {
        return false;
    }

    u32 rd = ex_req->inst.r.rd;
    u32 rs1 = ex_req->inst.r.rs1;

    ldst_req->st = false;
    ldst_req->size = LDST_REQ_SIZE_B4;
    ldst_req->addr = get_gpr(exu, rs1);
    ldst_req->strobe = 0b1111;

    exu->amo_rd = rd;
    exu->amo_addr = ldst_req->addr;
    exu->amo_stage = AMO_STAGE_READ;

    DBG_LOG(LOG_TRACE, "lr.w %s, (%s) # 0x%08x\n",
        gpr_name(rd), gpr_name(rs1), ldst_req->addr);

    return true;
}

DECL_AMO_BIU_RSP_HANDLER(lr)
{
    if (exu->amo_stage == AMO_STAGE_READ) {
        exu->amo_lr_set = true;
        exu->amo_rsvd_addr = exu->amo_addr;

        set_gpr(exu, exu->amo_rd, ldst_rsp->data);
        exu->amo_stage = AMO_STAGE_IDLE;
        DBG_LOG(LOG_TRACE, "lr.w completed: %s = 0x%08x\n",
            gpr_name(exu->amo_rd), ldst_rsp->data);
    } else {
        DBG_CHECK(0);
    }

    return false;
}

DECL_AMO_EX_REQ_HANDLER(sc)
{
    u32 rd = ex_req->inst.r.rd;
    u32 rs1 = ex_req->inst.r.rs1;
    u32 rs2 = ex_req->inst.r.rs2;
    u32 store_addr = get_gpr(exu, rs1);
    exu->amo_rd = rd;

    bool success = exu->amo_lr_set && (exu->amo_rsvd_addr == store_addr);
    if (success && (!biu_rdy)) {
        return false;
    }

    DBG_LOG(LOG_TRACE, "sc.w %s, %s, (%s) # 0x%08x\n",
        gpr_name(rd), gpr_name(rs2), gpr_name(rs1), ldst_req->addr);

    if (!success) {
        set_gpr(exu, exu->amo_rd, 1);
        exu->amo_lr_set = false;
        exu->amo_stage = AMO_STAGE_IDLE;
        DBG_LOG(LOG_TRACE, "sc.w completed (fail)\n");
        return false;
    }

    ldst_req->st = true;
    ldst_req->size = LDST_REQ_SIZE_B4;
    ldst_req->addr = store_addr;
    ldst_req->data = get_gpr(exu, rs2);
    ldst_req->strobe = 0b1111;

    exu->amo_stage = AMO_STAGE_WRITE;
    return true;
}

DECL_AMO_BIU_RSP_HANDLER(sc)
{
    if (exu->amo_stage == AMO_STAGE_WRITE) {
        set_gpr(exu, exu->amo_rd, 0);
        exu->amo_lr_set = false;
        exu->amo_stage = AMO_STAGE_IDLE;
        DBG_LOG(LOG_TRACE, "sc.w completed (success)\n");
    } else {
        DBG_CHECK(0);
    }

    return false;
}

#define AMO_EX_REQ_HANDLER_TEMPLATE(inst_name) do { \
    if (!biu_rdy) { \
        return false; \
    } \
    u32 rd = ex_req->inst.r.rd; \
    u32 rs1 = ex_req->inst.r.rs1; \
    u32 rs2 = ex_req->inst.r.rs2; \
    \
    ldst_req->st = false; \
    ldst_req->size = LDST_REQ_SIZE_B4; \
    ldst_req->addr = get_gpr(exu, rs1); \
    ldst_req->strobe = 0b1111; \
    \
    exu->amo_rd = rd; \
    exu->amo_addr = ldst_req->addr; \
    exu->amo_s2.u = get_gpr(exu, rs2); \
    exu->amo_stage = AMO_STAGE_READ; \
    \
    DBG_LOG(LOG_TRACE, #inst_name" %s, %s, (%s) # 0x%08x\n", \
        gpr_name(rd), gpr_name(rs2), gpr_name(rs1), ldst_req->addr); \
    return true; \
} while (0)

#define AMO_BIU_RSP_HANDLER_TEMPLATE(inst_name, expr) do { \
    if (exu->amo_stage == AMO_STAGE_READ) { \
        if (!biu_rdy) { \
            return false; \
        } \
        i32 old_val, new_val; \
        old_val.u = ldst_rsp->data; \
        set_gpr(exu, exu->amo_rd, old_val.u); \
        expr; \
        \
        st_req->st = true; \
        st_req->size = LDST_REQ_SIZE_B4; \
        st_req->addr = exu->amo_addr; \
        st_req->data = new_val.u; \
        st_req->strobe = 0b1111; \
        \
        exu->amo_stage = AMO_STAGE_WRITE; \
        return true; \
    } else if (exu->amo_stage == AMO_STAGE_WRITE) { \
        exu->amo_stage = AMO_STAGE_IDLE; \
        return false; \
    } else { \
        DBG_CHECK(0); \
        return false; \
    } \
} while (0)

DECL_AMO_EX_REQ_HANDLER(amoswap)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amoswap);
}

DECL_AMO_BIU_RSP_HANDLER(amoswap)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amoswap, new_val.u = exu->amo_s2.u);
}

DECL_AMO_EX_REQ_HANDLER(amoadd)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amoadd);
}

DECL_AMO_BIU_RSP_HANDLER(amoadd)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amoadd, new_val.s = old_val.s + exu->amo_s2.s);
}

DECL_AMO_EX_REQ_HANDLER(amoxor)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amoxor);
}

DECL_AMO_BIU_RSP_HANDLER(amoxor)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amoxor, new_val.u = old_val.u ^ exu->amo_s2.u);
}

DECL_AMO_EX_REQ_HANDLER(amoand)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amoand);
}

DECL_AMO_BIU_RSP_HANDLER(amoand)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amoand, new_val.u = old_val.u & exu->amo_s2.u);
}

DECL_AMO_EX_REQ_HANDLER(amoor)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amoor);
}

DECL_AMO_BIU_RSP_HANDLER(amoor)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amoor, new_val.u = old_val.u | exu->amo_s2.u);
}

DECL_AMO_EX_REQ_HANDLER(amomin)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amomin);
}

DECL_AMO_BIU_RSP_HANDLER(amomin)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amomin, new_val.s = MIN(old_val.s, exu->amo_s2.s));
}

DECL_AMO_EX_REQ_HANDLER(amomax)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amomax);
}

DECL_AMO_BIU_RSP_HANDLER(amomax)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amomax, new_val.s = MAX(old_val.s, exu->amo_s2.s));
}

DECL_AMO_EX_REQ_HANDLER(amominu)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amominu);
}

DECL_AMO_BIU_RSP_HANDLER(amominu)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amominu, new_val.u = MIN(old_val.u, exu->amo_s2.u));
}

DECL_AMO_EX_REQ_HANDLER(amomaxu)
{
    AMO_EX_REQ_HANDLER_TEMPLATE(amomaxu);
}

DECL_AMO_BIU_RSP_HANDLER(amomaxu)
{
    AMO_BIU_RSP_HANDLER_TEMPLATE(amomaxu, new_val.u = MAX(old_val.u, exu->amo_s2.u));
}

void amo_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    DBG_CHECK(ex_req->inst.base.opcode == OPCODE_AMO);

    static amo_ex_req_handler_t handlers[256] = {
        [RV32G_AMO_FUNCT375_LR] = GET_AMO_EX_REQ_HANDLER(lr),
        [RV32G_AMO_FUNCT375_SC] = GET_AMO_EX_REQ_HANDLER(sc),
        [RV32G_AMO_FUNCT375_AMOSWAP] = GET_AMO_EX_REQ_HANDLER(amoswap),
        [RV32G_AMO_FUNCT375_AMOADD] = GET_AMO_EX_REQ_HANDLER(amoadd),
        [RV32G_AMO_FUNCT375_AMOXOR] = GET_AMO_EX_REQ_HANDLER(amoxor),
        [RV32G_AMO_FUNCT375_AMOAND] = GET_AMO_EX_REQ_HANDLER(amoand),
        [RV32G_AMO_FUNCT375_AMOOR] = GET_AMO_EX_REQ_HANDLER(amoor),
        [RV32G_AMO_FUNCT375_AMOMIN] = GET_AMO_EX_REQ_HANDLER(amomin),
        [RV32G_AMO_FUNCT375_AMOMAX] = GET_AMO_EX_REQ_HANDLER(amomax),
        [RV32G_AMO_FUNCT375_AMOMINU] = GET_AMO_EX_REQ_HANDLER(amominu),
        [RV32G_AMO_FUNCT375_AMOMAXU] = GET_AMO_EX_REQ_HANDLER(amomaxu)
    };

    bool send_ldst_req;
    ldst_req_if_t ldst_req = {
        .cmo = LDST_REQ_CMO_NONE
    };
    u32 funct375 = (ex_req->inst.r.funct3 << 5) | (ex_req->inst.r.funct7 >> 2);
    amo_ex_req_handler_t handler = handlers[funct375];
    bool biu_rdy = !itf_fifo_full(exu->ldst_req_mst);

    if (handler) {
        send_ldst_req = handler(exu, ex_req, &ldst_req, biu_rdy);
    } else {
        DBG_CHECK(0);
    }

    if (send_ldst_req) {
        itf_write(exu->ldst_req_mst, &ldst_req);
    }

    exu->cur_opcode = OPCODE_AMO;
    exu->amo_funct375 = funct375;
    exu->trap_defer = (exu->amo_stage != AMO_STAGE_IDLE);
}

void amo_biu_rsp_proc(exu_t *exu, const ldst_rsp_if_t *ldst_rsp)
{
    DBG_CHECK(exu->cur_opcode == OPCODE_AMO);

    static amo_biu_rsp_handler_t handlers[256] = {
        [RV32G_AMO_FUNCT375_LR] = GET_AMO_BIU_RSP_HANDLER(lr),
        [RV32G_AMO_FUNCT375_SC] = GET_AMO_BIU_RSP_HANDLER(sc),
        [RV32G_AMO_FUNCT375_AMOSWAP] = GET_AMO_BIU_RSP_HANDLER(amoswap),
        [RV32G_AMO_FUNCT375_AMOADD] = GET_AMO_BIU_RSP_HANDLER(amoadd),
        [RV32G_AMO_FUNCT375_AMOXOR] = GET_AMO_BIU_RSP_HANDLER(amoxor),
        [RV32G_AMO_FUNCT375_AMOAND] = GET_AMO_BIU_RSP_HANDLER(amoand),
        [RV32G_AMO_FUNCT375_AMOOR] = GET_AMO_BIU_RSP_HANDLER(amoor),
        [RV32G_AMO_FUNCT375_AMOMIN] = GET_AMO_BIU_RSP_HANDLER(amomin),
        [RV32G_AMO_FUNCT375_AMOMAX] = GET_AMO_BIU_RSP_HANDLER(amomax),
        [RV32G_AMO_FUNCT375_AMOMINU] = GET_AMO_BIU_RSP_HANDLER(amominu),
        [RV32G_AMO_FUNCT375_AMOMAXU] = GET_AMO_BIU_RSP_HANDLER(amomaxu)
    };

    bool send_st_req;
    ldst_req_if_t st_req = {
        .cmo = LDST_REQ_CMO_NONE
    };
    amo_biu_rsp_handler_t handler = handlers[exu->amo_funct375];
    bool biu_rdy = !itf_fifo_full(exu->ldst_req_mst);

    if (handler) {
        send_st_req = handler(exu, ldst_rsp, &st_req, biu_rdy);
    } else {
        DBG_CHECK(0);
    }

    if (send_st_req) {
        itf_write(exu->ldst_req_mst, &st_req);
    }
    exu->trap_defer = (exu->amo_stage != AMO_STAGE_IDLE);
    if (!exu->trap_defer) {
        exu->irq_epc = exu->cur_pc + 4;
    }
}
