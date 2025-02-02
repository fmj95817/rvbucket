#include "exu.h"
#include "utils.h"
#include "dbg.h"

#define DECL_SYS_HANDLER(inst) static void inst##_ex_req_handler(exu_t *exu, const ex_req_if_t *req)
#define CALL_SYS_HANDLER(inst, exu, req) inst##_ex_req_handler(exu, req)

DECL_SYS_HANDLER(fence)
{
    DBG_LOG(LOG_TRACE, "fence\n");
}

DECL_SYS_HANDLER(ecall)
{
    DBG_LOG(LOG_TRACE, "ecall\n");
}

DECL_SYS_HANDLER(ebreak)
{
    DBG_LOG(LOG_TRACE, "ebreak\n");
}

DECL_SYS_HANDLER(sys_group)
{
    if (req->inst.i.imm_11_0 == 0b000000000000) {
        CALL_SYS_HANDLER(ecall, exu, req);
    } else if (req->inst.i.imm_11_0 == 0b000000000001) {
        CALL_SYS_HANDLER(ebreak, exu, req);
    } else {
        DBG_CHECK(0);
    }
}

void sys_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req)
{
    if (ex_req->inst.base.opcode == OPCODE_MEM) {
        CALL_SYS_HANDLER(fence, exu, ex_req);
    } else if (ex_req->inst.base.opcode == OPCODE_SYSTEM) {
        CALL_SYS_HANDLER(sys_group, exu, ex_req);
    } else {
        DBG_CHECK(0);
    }
}