#ifndef DBG_PCM_H
#define DBG_PCM_H

#include "base/types.h"

extern u64 *dbg_pcm_reg_perf_cnt(const char *hier_name, const char *name);
extern void dbg_pcm_set_cycle(const u64 *cycle);
extern void dbg_pcm_clock(void);

#endif
