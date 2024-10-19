#ifndef DBG_H
#define DBG_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define DBG_CHECK(expr) assert(expr)

#define DBG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define DBG_FPRINT(fp, fmt, ...) \
do { \
    if (fp != NULL) { \
        fprintf(fp, fmt, ##__VA_ARGS__); \
    } \
} while (0)

typedef struct log_sys {
    FILE *trace;
} log_sys_t;

#endif