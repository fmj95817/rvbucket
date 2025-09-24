#include "exu.h"
#include "utils.h"
#include "dbg/chk.h"
#include "dbg/log.h"

#define DECL_SYS_HANDLER(inst) static void inst##_ex_req_handler(exu_t *exu, const ex_req_if_t *req)
#define CALL_SYS_HANDLER(inst, exu, req) inst##_ex_req_handler(exu, req)
#define GET_SYS_HANDLER(inst) &inst##_ex_req_handler

typedef void (*sys_ex_req_handler_t)(exu_t *exu, const ex_req_if_t *req);

DECL_SYS_HANDLER(ecall)
{
    DBG_LOG(LOG_TRACE, "ecall\n");
}

DECL_SYS_HANDLER(ebreak)
{
    DBG_LOG(LOG_TRACE, "ebreak\n");
}

DECL_SYS_HANDLER(priv_group)
{
    if (req->inst.i.imm_11_0 == 0b000000000000) {
        CALL_SYS_HANDLER(ecall, exu, req);
    } else if (req->inst.i.imm_11_0 == 0b000000000001) {
        CALL_SYS_HANDLER(ebreak, exu, req);
    } else {
        DBG_CHECK(0);
    }
}

DECL_SYS_HANDLER(csrrw)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu->csr, *exu->priv, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 new_val = get_gpr(exu, req->inst.i.rs1);
    DBG_CHECK(csr_write(exu->csr, *exu->priv, req->inst.i.imm_11_0, new_val));

    DBG_LOG(LOG_TRACE, "csrrw, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        csr_name(req->inst.i.imm_11_0), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrs)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu->csr, *exu->priv, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = get_gpr(exu, req->inst.i.rs1);
    DBG_CHECK(csr_write(exu->csr, *exu->priv, req->inst.i.imm_11_0, old_val | mask));

    DBG_LOG(LOG_TRACE, "csrrs, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        csr_name(req->inst.i.imm_11_0), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrc)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu->csr, *exu->priv, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = get_gpr(exu, req->inst.i.rs1);
    DBG_CHECK(csr_write(exu->csr, *exu->priv, req->inst.i.imm_11_0, old_val & (~mask)));

    DBG_LOG(LOG_TRACE, "csrrc, %s, %s, %s\n", gpr_name(req->inst.i.rd),
        csr_name(req->inst.i.imm_11_0), gpr_name(req->inst.i.rs1));
}

DECL_SYS_HANDLER(csrrwi)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu->csr, *exu->priv, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 new_val = req->inst.i.rs1;
    DBG_CHECK(csr_write(exu->csr, *exu->priv, req->inst.i.imm_11_0, new_val));

    DBG_LOG(LOG_TRACE, "csrrwi, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        csr_name(req->inst.i.imm_11_0), req->inst.i.rs1);
}

DECL_SYS_HANDLER(csrrsi)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu->csr, *exu->priv, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = req->inst.i.rs1;
    DBG_CHECK(csr_write(exu->csr, *exu->priv, req->inst.i.imm_11_0, old_val | mask));

    DBG_LOG(LOG_TRACE, "csrrsi, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        csr_name(req->inst.i.imm_11_0), req->inst.i.rs1);
}

DECL_SYS_HANDLER(csrrci)
{
    u32 old_val;
    DBG_CHECK(csr_read(exu->csr, *exu->priv, req->inst.i.imm_11_0, &old_val));
    set_gpr(exu, req->inst.i.rd, old_val);

    u32 mask = req->inst.i.rs1;
    DBG_CHECK(csr_write(exu->csr, *exu->priv, req->inst.i.imm_11_0, old_val & (~mask)));

    DBG_LOG(LOG_TRACE, "csrrci, %s, %s, %u\n", gpr_name(req->inst.i.rd),
        csr_name(req->inst.i.imm_11_0), req->inst.i.rs1);
}

DECL_SYS_HANDLER(sys_group)
{
    static sys_ex_req_handler_t sys_handlers[8] = {
        [SYSTEM_FUNCT3_PRIV] = GET_SYS_HANDLER(priv_group),
        [SYSTEM_FUNCT3_CSRRW] = GET_SYS_HANDLER(csrrw),
        [SYSTEM_FUNCT3_CSRRS] = GET_SYS_HANDLER(csrrs),
        [SYSTEM_FUNCT3_CSRRC] = GET_SYS_HANDLER(csrrc),
        [SYSTEM_FUNCT3_CSRRWI] = GET_SYS_HANDLER(csrrwi),
        [SYSTEM_FUNCT3_CSRRSI] = GET_SYS_HANDLER(csrrsi),
        [SYSTEM_FUNCT3_CSRRCI] = GET_SYS_HANDLER(csrrci)
    };

    sys_ex_req_handler_t handler = sys_handlers[req->inst.i.funct3];
    if (handler) {
        handler(exu, req);
    } else {
        DBG_CHECK(0);
    }
}

void sys_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    if (ex_req->inst.base.opcode == OPCODE_SYSTEM) {
        CALL_SYS_HANDLER(sys_group, exu, ex_req);
    } else {
        DBG_CHECK(0);
    }
}