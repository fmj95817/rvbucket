#include "pcm.h"
#include <string.h>
#include <stdio.h>
#include "base/def.h"
#include "base/mod.h"
#include "dbg/chk.h"
#include "dbg/env.h"

#define DBG_PERF_CNT_MAX 128
#define DBG_PERF_CNT_NAME_MAX (HIER_NAME_MAX + 128u)

typedef struct perf_cnt {
    char name[DBG_PERF_CNT_NAME_MAX];
    u64 val;
} perf_cnt_t;

typedef struct dbg_pcm {
    u32 dump_flags;
    u32 nxt_entry_idx;
    perf_cnt_t perf_cnts[DBG_PERF_CNT_MAX];
} dbg_pcm_t;

typedef enum dump_dst {
    DUMP_TO_STDOUT_BIT = (1 << 0),
    DUMP_TO_CSV_BIT = (1 << 1)
} dump_dst_t;

static dbg_pcm_t g_pcm;

__attribute__((constructor)) void dbg_pcm_constructor()
{
    g_pcm.dump_flags = dbg_get_uint_env("PCD");
    g_pcm.nxt_entry_idx = 0;
    memset(g_pcm.perf_cnts, 0, sizeof(g_pcm.perf_cnts));
}

static void dump_to_csv()
{
    FILE *csv_fp = fopen("perf.csv", "w");
    if (csv_fp == NULL) {
        return;
    }

    fprintf(csv_fp, "name, value\n");
    for (u32 i = 0; i < g_pcm.nxt_entry_idx; i++) {
        fprintf(csv_fp, "%s, %"U64_FMT"\n", g_pcm.perf_cnts[i].name, g_pcm.perf_cnts[i].val);
    }

    fclose(csv_fp);
}

static void dump_to_stdout()
{
    for (u32 i = 0; i < g_pcm.nxt_entry_idx; i++) {
        printf("%s = %"U64_FMT"\n", g_pcm.perf_cnts[i].name, g_pcm.perf_cnts[i].val);
    }
}

__attribute__((destructor)) void dbg_pcm_destructor()
{
    if (g_pcm.dump_flags & DUMP_TO_STDOUT_BIT) {
        dump_to_stdout();
    }

    if (g_pcm.dump_flags & DUMP_TO_CSV_BIT) {
        dump_to_csv();
    }
}

u64 *dbg_pcm_reg_perf_cnt(const char *hier_name, const char *name)
{
    DBG_CHECK(hier_name != NULL);
    DBG_CHECK(name != NULL);
    char full_name[DBG_PERF_CNT_NAME_MAX];
    int ret = snprintf(full_name, sizeof(full_name),
        "%s.%s", hier_name, name);
    DBG_CHECK(ret >= 0 && (u32)ret < sizeof(full_name));

    for (u32 i = 0; i < g_pcm.nxt_entry_idx; i++) {
        if (strcmp(g_pcm.perf_cnts[i].name, full_name) == 0) {
            return &g_pcm.perf_cnts[i].val;
        }
    }

    u32 idx = g_pcm.nxt_entry_idx;
    DBG_CHECK(idx < DBG_PERF_CNT_MAX);
    strcpy(g_pcm.perf_cnts[idx].name, full_name);
    g_pcm.perf_cnts[idx].val = 0;
    u64 *val = &g_pcm.perf_cnts[idx].val;
    g_pcm.nxt_entry_idx++;

    return val;
}
