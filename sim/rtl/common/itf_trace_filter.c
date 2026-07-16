#ifndef VERILATOR

#include <ctype.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "svdpi.h"

#define ITF_TRACE_FILTER_ENV "ITF_TRACE_FILTER"
#define ITF_TRACE_FILTER_MAX 1024

typedef bool (*itf_trace_filter_t)(const void *);

typedef struct itf_trace_filter_ctx {
    itf_trace_filter_t filter;
} itf_trace_filter_ctx_t;

static void *g_filter_so;
static bool g_filter_so_inited;
static itf_trace_filter_ctx_t *g_filters[ITF_TRACE_FILTER_MAX];
static uint32_t g_filter_num = 1;

static void itf_trace_filter_cleanup(void)
{
    for (uint32_t i = 1; i < g_filter_num; i++) {
        free(g_filters[i]);
    }
    if (g_filter_so != NULL) {
        dlclose(g_filter_so);
    }
}

static void itf_trace_filter_so_init(void)
{
    if (g_filter_so_inited) {
        return;
    }

    g_filter_so_inited = true;
    const char *path = getenv(ITF_TRACE_FILTER_ENV);
    if (path == NULL || path[0] == '\0') {
        return;
    }

    g_filter_so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (g_filter_so == NULL) {
        fprintf(stderr, "failed to load interface trace filter %s: %s\n",
            path, dlerror());
        abort();
    }
    atexit(itf_trace_filter_cleanup);
}

static void itf_trace_filter_symbol(char *symbol, size_t symbol_size,
    const char *name)
{
    size_t out = 0;
    for (size_t i = 0; name[i] != '\0'; i++) {
        unsigned char ch = (unsigned char)name[i];
        if (ch == '[') {
            if (out != 0 && symbol[out - 1] != '_') {
                if (out + 1 >= symbol_size) {
                    goto too_long;
                }
                symbol[out++] = '_';
            }
            while (isdigit((unsigned char)name[i + 1])) {
                if (out + 1 >= symbol_size) {
                    goto too_long;
                }
                symbol[out++] = name[++i];
            }
            if (name[i + 1] == ']') {
                i++;
            }
        } else if (isalnum(ch) || ch == '_') {
            if (out + 1 >= symbol_size) {
                goto too_long;
            }
            symbol[out++] = (char)ch;
        } else if (out != 0 && symbol[out - 1] != '_') {
            if (out + 1 >= symbol_size) {
                goto too_long;
            }
            symbol[out++] = '_';
        }
    }
    if (snprintf(&symbol[out], symbol_size - out, "_filter") >=
        (int)(symbol_size - out)) {
        goto too_long;
    }
    return;

too_long:
    fprintf(stderr, "interface trace filter symbol is too long: %s\n",
        name);
    abort();
}

int rvb_itf_trace_filter_open(const char *name, int payload_width)
{
    (void)payload_width;
    itf_trace_filter_so_init();
    if (g_filter_so == NULL) {
        return 0;
    }

    char symbol[1024];
    itf_trace_filter_symbol(symbol, sizeof(symbol), name);
    dlerror();
    void *filter = dlsym(g_filter_so, symbol);
    if (dlerror() != NULL) {
        return 0;
    }

    if (g_filter_num >= ITF_TRACE_FILTER_MAX) {
        fprintf(stderr, "too many interface trace filters\n");
        abort();
    }

    itf_trace_filter_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        fprintf(stderr, "failed to allocate interface trace filter\n");
        abort();
    }
    ctx->filter = (itf_trace_filter_t)filter;

    uint32_t filter_id = g_filter_num;
    g_filters[g_filter_num++] = ctx;
    return (int)filter_id;
}

svBit rvb_itf_trace_filter_eval(int filter_id, const svBitVecVal *payload)
{
    if (filter_id <= 0 || (uint32_t)filter_id >= g_filter_num) {
        return 1;
    }

    itf_trace_filter_ctx_t *ctx = g_filters[filter_id];
    return ctx->filter(payload) ? 1 : 0;
}

#endif
