#include "mod.h"
#include <stdio.h>
#include "dbg/chk.h"

void mod_construct(mod_t *mod, const char *parent, const char *name)
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
}

void mod_free(mod_t *mod)
{
    (void)mod;
}

void mod_reset(mod_t *mod)
{
    (void)mod;
}

void mod_clock(mod_t *mod)
{
    (void)mod;
}
