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

void misc_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    if (ex_req->inst.base.opcode == OPCODE_LUI) {
        CALL_MISC_HANDLER(lui, exu, ex_req);
    } else if (ex_req->inst.base.opcode == OPCODE_AUIPC) {
        CALL_MISC_HANDLER(auipc, exu, ex_req);
    } else {
        DBG_CHECK(0);
    }
}
