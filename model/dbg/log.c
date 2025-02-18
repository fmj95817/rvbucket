#include "log.h"
#include "env.h"
#include "base/types.h"

typedef struct log_module_attr {
    const char *path;
    const char *env_switch;
    FILE *fp;
    bool enable;
} log_module_attr_t;

static log_module_attr_t g_log_mod_attrs[LOG_MOD_COUNT] = {
    [LOG_PRINT] = { NULL, NULL, NULL, true },
    [LOG_TRACE] = { "trace.txt", "GEN_TRACE", NULL, true }
};

__attribute__((constructor)) void dbg_log_constructor()
{
    for (int i = 0; i < LOG_MOD_COUNT; i++) {
        log_module_attr_t *mod = g_log_mod_attrs + i;
        if (mod->path == NULL) {
            mod->fp = stdout;
            continue;
        }

        bool gen_file = dbg_get_bool_env(mod->env_switch);
        if (mod->env_switch != NULL && (!gen_file)) {
            continue;
        }

        mod->fp = fopen(mod->path, "w");
    }
}

__attribute__((destructor)) void dbg_log_destructor()
{
    for (int i = 0; i < LOG_MOD_COUNT; i++) {
        log_module_attr_t *mod = g_log_mod_attrs + i;
        if (mod->fp != NULL && mod->path != NULL) {
            fclose(mod->fp);
            mod->fp = NULL;
        }
    }
}

FILE *dbg_get_log_module_fp(log_module_t mod)
{
    log_module_attr_t *attr = g_log_mod_attrs + mod;
    return attr->enable ? attr->fp : NULL;
}

void dbg_disable_log_module(log_module_t mod)
{
    log_module_attr_t *attr = g_log_mod_attrs + mod;
    attr->enable = false;
}

void dbg_enable_log_module(log_module_t mod)
{
    log_module_attr_t *attr = g_log_mod_attrs + mod;
    attr->enable = true;
}