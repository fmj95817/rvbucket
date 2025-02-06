#ifndef DBG_H
#define DBG_H

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "base/types.h"

#define DBG_CHECK(expr) assert(expr)

typedef enum log_module {
    LOG_PRINT = 0,
    LOG_TRACE = 1,
    LOG_MOD_COUNT = LOG_TRACE + 1
} log_module_t;

extern FILE *dbg_get_log_module_fp(log_module_t mod);
extern void dbg_disable_log_module(log_module_t mod);
extern void dbg_enable_log_module(log_module_t mod);
extern bool dbg_get_bool_env(const char *key);

#define DBG_LOG(mod, fmt, ...) \
do { \
    FILE *fp = dbg_get_log_module_fp(mod); \
    if (fp != NULL) { \
        fprintf(fp, fmt, ##__VA_ARGS__); \
        fflush(fp); \
    } \
} while (0)

#endif