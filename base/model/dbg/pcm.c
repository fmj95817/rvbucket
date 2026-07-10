#include "pcm.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "base/def.h"
#include "base/mod.h"
#include "dbg/chk.h"
#include "dbg/env.h"

#define DBG_PERF_CNT_MAX 128
#define DBG_PERF_CNT_NAME_MAX (HIER_NAME_MAX + 128u)
#define DBG_PCM_SAMPLE_CAP_INIT 64u
#define DBG_PCM_PERIOD_ENV "PCD_PERIOD"
#define DBG_PCM_PERIOD_DEFAULT 10000000u
#define DBG_PCM_PERIODIC_CSV "perf_periodic.csv"

typedef struct perf_cnt {
    char name[DBG_PERF_CNT_NAME_MAX];
    u64 val;
} perf_cnt_t;

typedef struct perf_sample {
    u64 cycle;
    u64 vals[DBG_PERF_CNT_MAX];
} perf_sample_t;

typedef struct dbg_pcm {
    u32 dump_flags;
    u32 nxt_entry_idx;
    const u64 *cycle;
    u64 sample_period;
    u64 next_sample_cycle;
    u32 sample_count;
    u32 sample_cap;
    perf_sample_t *samples;
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
    g_pcm.cycle = NULL;
    g_pcm.sample_period = dbg_get_uint_env(DBG_PCM_PERIOD_ENV);
    if (g_pcm.dump_flags != 0 && g_pcm.sample_period == 0) {
        g_pcm.sample_period = DBG_PCM_PERIOD_DEFAULT;
    }
    g_pcm.next_sample_cycle = g_pcm.sample_period;
    g_pcm.sample_count = 0;
    g_pcm.sample_cap = 0;
    g_pcm.samples = NULL;
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

static void ensure_sample_capacity(void)
{
    if (g_pcm.sample_count < g_pcm.sample_cap) {
        return;
    }

    u32 new_cap = g_pcm.sample_cap == 0 ?
        DBG_PCM_SAMPLE_CAP_INIT : g_pcm.sample_cap * 2u;
    perf_sample_t *new_samples = realloc(g_pcm.samples,
        sizeof(*g_pcm.samples) * new_cap);
    DBG_CHECK(new_samples != NULL);
    g_pcm.samples = new_samples;
    g_pcm.sample_cap = new_cap;
}

static void capture_periodic_sample(u64 cycle)
{
    ensure_sample_capacity();

    perf_sample_t *sample = &g_pcm.samples[g_pcm.sample_count];
    sample->cycle = cycle;
    memset(sample->vals, 0, sizeof(sample->vals));
    for (u32 i = 0; i < g_pcm.nxt_entry_idx; i++) {
        sample->vals[i] = g_pcm.perf_cnts[i].val;
    }
    g_pcm.sample_count++;
}

static void dump_periodic_table(void)
{
    FILE *csv_fp = fopen(DBG_PCM_PERIODIC_CSV, "w");
    if (csv_fp == NULL) {
        return;
    }

    fprintf(csv_fp, "counter");
    for (u32 sample = 0; sample < g_pcm.sample_count; sample++) {
        fprintf(csv_fp, ",%"U64_FMT, g_pcm.samples[sample].cycle);
    }
    fprintf(csv_fp, "\n");

    for (u32 cnt = 0; cnt < g_pcm.nxt_entry_idx; cnt++) {
        fprintf(csv_fp, "%s", g_pcm.perf_cnts[cnt].name);
        for (u32 sample = 0; sample < g_pcm.sample_count; sample++) {
            fprintf(csv_fp, ",%"U64_FMT, g_pcm.samples[sample].vals[cnt]);
        }
        fprintf(csv_fp, "\n");
    }

    fclose(csv_fp);
}

__attribute__((destructor)) void dbg_pcm_destructor()
{
    if (g_pcm.dump_flags & DUMP_TO_STDOUT_BIT) {
        dump_to_stdout();
    }

    if (g_pcm.dump_flags & DUMP_TO_CSV_BIT) {
        dump_to_csv();
    }

    if (g_pcm.sample_count > 0) {
        dump_periodic_table();
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

void dbg_pcm_set_cycle(const u64 *cycle)
{
    g_pcm.cycle = cycle;
    if (cycle != NULL) {
        g_pcm.next_sample_cycle = *cycle + g_pcm.sample_period;
    }
}

void dbg_pcm_clock(void)
{
    if (g_pcm.dump_flags == 0 || g_pcm.sample_period == 0 ||
        g_pcm.cycle == NULL) {
        return;
    }

    u64 cycle = *g_pcm.cycle;
    if (cycle < g_pcm.next_sample_cycle) {
        return;
    }

    capture_periodic_sample(cycle);
    dump_periodic_table();
    do {
        g_pcm.next_sample_cycle += g_pcm.sample_period;
    } while (g_pcm.next_sample_cycle <= cycle);
}
