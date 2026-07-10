#ifndef MOD_H
#define MOD_H

#include "types.h"

#define HIER_NAME_MAX 256u

typedef void (*mod_clock_func_t)(void *);
typedef struct smp_opt smp_opt_t;

typedef struct mod {
    const u64 *cycle;
    char hier_name[HIER_NAME_MAX];
    smp_opt_t *smp;
} mod_t;

extern void mod_construct(mod_t *mod, const char *parent, const char *name);
extern void mod_construct_smp_opt(mod_t *mod, const char *parent,
    const char *name, mod_clock_func_t clock_func, void *clock_args,
    bool threaded);
extern void mod_free(mod_t *mod);
extern void mod_reset(mod_t *mod);
extern void mod_clock(mod_t *mod);
extern bool mod_clock_async(mod_t *mod);

#endif
