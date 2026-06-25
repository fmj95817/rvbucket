#include "utils.h"
#include "dbg/log.h"

#define CLR_GREEN  "\033[32m"
#define CLR_RED    "\033[31m"
#define CLR_YELLOW "\033[33m"
#define CLR_CYAN   "\033[36m"
#define CLR_RESET  "\033[0m"

void ut_sbd_init(ut_sbd_t *sbd)
{
    sbd->pass = 0;
    sbd->fail = 0;
    sbd->test_pass = 0;
    sbd->test_fail = 0;
    sbd->test_name = NULL;
}

void ut_sbd_require(ut_sbd_t *sbd, bool cond, const char *msg, u32 line)
{
    if (cond) {
        sbd->pass++;
        sbd->test_pass++;
        DBG_LOG(LOG_PRINT, "  [" CLR_GREEN "PASS" CLR_RESET "] %s\n", msg);
    } else {
        sbd->fail++;
        sbd->test_fail++;
        DBG_LOG(LOG_PRINT, "  [" CLR_RED "FAIL" CLR_RESET "] %s (line %u)\n", msg, line);
    }
}

void ut_sbd_begin(ut_sbd_t *sbd, const char *desc)
{
    sbd->test_name = desc;
    sbd->test_pass = 0;
    sbd->test_fail = 0;
    DBG_LOG(LOG_PRINT, "\n" CLR_CYAN "══════════════════════════════════════════" CLR_RESET "\n");
    DBG_LOG(LOG_PRINT, CLR_CYAN "  Test: %s" CLR_RESET "\n", desc);
    DBG_LOG(LOG_PRINT, CLR_CYAN "══════════════════════════════════════════" CLR_RESET "\n\n");
}

void ut_sbd_end(ut_sbd_t *sbd)
{
    DBG_LOG(LOG_PRINT, "\n  ── Result: %s ── ", sbd->test_name);
    DBG_LOG(LOG_PRINT, CLR_GREEN "%u passed" CLR_RESET, sbd->test_pass);
    if (sbd->test_fail > 0) {
        DBG_LOG(LOG_PRINT, ", " CLR_RED "%u failed" CLR_RESET, sbd->test_fail);
    }
    DBG_LOG(LOG_PRINT, "\n");
}

void ut_sbd_summary(ut_sbd_t *sbd)
{
    DBG_LOG(LOG_PRINT, "\n" CLR_YELLOW "══════════════════════════════════════════" CLR_RESET "\n");
    DBG_LOG(LOG_PRINT, CLR_YELLOW "  Total: " CLR_GREEN "%u passed" CLR_RESET, sbd->pass);
    if (sbd->fail > 0) {
        DBG_LOG(LOG_PRINT, ", " CLR_RED "%u failed" CLR_RESET, sbd->fail);
    }
    DBG_LOG(LOG_PRINT, "\n" CLR_YELLOW "══════════════════════════════════════════" CLR_RESET "\n");
}

int ut_sbd_ret(ut_sbd_t *sbd)
{
    return sbd->fail ? 1 : 0;
}

bool ut_poll_until(void *ctx, ut_cond_fn_t cond, ut_tick_fn_t tick, u32 timeout)
{
    for (u32 i = 0; i < timeout; i++) {
        if (cond(ctx)) {
            return true;
        }
        tick(ctx);
    }
    return cond(ctx);
}

void ut_run_cycles(void *ctx, ut_tick_fn_t tick, u32 n)
{
    for (u32 i = 0; i < n; i++) {
        tick(ctx);
    }
}
