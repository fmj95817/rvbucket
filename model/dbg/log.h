#ifndef DBG_LOG_H
#define DBG_LOG_H

#include <stdio.h>
#include "base/types.h"

typedef enum log_module {
    LOG_PRINT = 0,
    LOG_TRACE = 1,
    LOG_MOD_COUNT = LOG_TRACE + 1
} log_module_t;

extern FILE *dbg_get_log_module_fp(log_module_t mod);
extern void dbg_disable_log_module(log_module_t mod);
extern void dbg_enable_log_module(log_module_t mod);

#define DBG_LOG(mod, fmt, ...) \
do { \
    FILE *fp = dbg_get_log_module_fp(mod); \
    if (fp != NULL) { \
        fprintf(fp, fmt, ##__VA_ARGS__); \
        fflush(fp); \
    } \
} while (0)

#endif
