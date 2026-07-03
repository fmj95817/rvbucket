#ifndef MOD_H
#define MOD_H

#include "types.h"

#define HIER_NAME_MAX 256u

typedef struct mod {
    const u64 *cycle;
    char hier_name[HIER_NAME_MAX];
} mod_t;

extern void mod_construct(mod_t *mod, const char *parent, const char *name);
extern void mod_free(mod_t *mod);
extern void mod_reset(mod_t *mod);
extern void mod_clock(mod_t *mod);

#endif
