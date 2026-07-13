#ifndef PERI_PCM_H
#define PERI_PCM_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"

typedef struct pcm {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;

    u32 base_addr;
    u32 size;
} pcm_t;

extern void pcm_construct(pcm_t *pcm, const char *parent, const char *name,
    u32 base_addr, u32 size);
extern void pcm_reset(pcm_t *pcm);
extern void pcm_clock(pcm_t *pcm);
extern void pcm_free(pcm_t *pcm);

#endif
