#include "mod.h"
#include <stdio.h>
#include "base/smp_opt.h"
#include "dbg/chk.h"

static void mod_wait(mod_t *mod)
{
    DBG_CHECK(mod != NULL);
    if (mod->smp != NULL) {
        smp_opt_drain(mod->smp);
    }
}

void mod_construct(mod_t *mod, const char *parent, const char *name)
{
    mod_construct_smp_opt(mod, parent, name, NULL, NULL, false);
}

void mod_construct_smp_opt(mod_t *mod, const char *parent, const char *name,
    mod_clock_func_t clock_func, void *clock_args, bool threaded)
{
    DBG_CHECK(mod != NULL);
    DBG_CHECK(name != NULL);
    int len;
    if (parent == NULL || parent[0] == '\0') {
        len = snprintf(mod->hier_name, HIER_NAME_MAX, "%s", name);
    } else {
        len = snprintf(mod->hier_name, HIER_NAME_MAX, "%s.%s", parent, name);
    }
    DBG_CHECK(len >= 0 && (u32)len < HIER_NAME_MAX);

    if (threaded) {
        DBG_CHECK(clock_func != NULL);
        DBG_CHECK(mod->cycle != NULL);
        mod->smp = smp_opt_create(mod->cycle, clock_func, clock_args);
        mod->cycle = smp_opt_cycle(mod->smp);
    } else {
        mod->smp = NULL;
    }
}

void mod_free(mod_t *mod)
{
    DBG_CHECK(mod != NULL);
    smp_opt_destroy(mod->smp);
    mod->smp = NULL;
}

void mod_reset(mod_t *mod)
{
    mod_wait(mod);
}

void mod_clock(mod_t *mod)
{
    DBG_CHECK(mod != NULL);
}

bool mod_clock_async(mod_t *mod)
{
    DBG_CHECK(mod != NULL);
    if (mod->smp == NULL) {
        return false;
    }

    smp_opt_submit(mod->smp);
    return true;
}
