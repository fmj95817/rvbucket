#ifndef UT_UTILS_H
#define UT_UTILS_H

#include <stdio.h>
#include "base/types.h"

#define UT_TIMEOUT 1000

typedef struct ut_sbd {
    u32 pass;
    u32 fail;
    u32 test_pass;
    u32 test_fail;
    const char *test_name;
} ut_sbd_t;

extern void ut_sbd_init(ut_sbd_t *sbd);
extern void ut_sbd_require(ut_sbd_t *sbd, bool cond, const char *msg, u32 line);
extern void ut_sbd_begin(ut_sbd_t *sbd, const char *desc);
extern void ut_sbd_end(ut_sbd_t *sbd);
extern void ut_sbd_summary(ut_sbd_t *sbd);
extern int  ut_sbd_ret(ut_sbd_t *sbd);

#define TEST_CASE(tb_type, name) \
    static void test_##name(tb_type *tb)

#define TEST_RUN(name) \
    test_##name(&tb)

#define TEST_BEGIN(desc) \
    ut_sbd_begin(&tb->sbd, desc)

#define TEST_END() \
    ut_sbd_end(&tb->sbd)

#define REQUIRE(cond, msg) \
    ut_sbd_require(&tb->sbd, (cond), (msg), __LINE__)

#define RUN_CYCLES(n) \
    ut_run_cycles(tb, (ut_tick_fn_t)tb_clock, (n))

#define RUN_POLL_UNTIL(cond, timeout) \
    ut_poll_until(tb, (ut_cond_fn_t)(cond), (ut_tick_fn_t)tb_clock, (timeout))

typedef bool (*ut_cond_fn_t)(void *);
typedef void (*ut_tick_fn_t)(void *);

extern bool ut_poll_until(void *ctx, ut_cond_fn_t cond, ut_tick_fn_t tick, u32 timeout);
extern void ut_run_cycles(void *ctx, ut_tick_fn_t tick, u32 n);

#endif
