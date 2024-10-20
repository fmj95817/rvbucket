#include "dbg.h"
#include <string.h>

typedef struct log_module_attr {
    bool enable;
    const char *path;
    const char *env_switch;
    FILE *fp;
} log_module_attr_t;

static log_module_attr_t g_log_mod_attrs[LOG_MOD_COUNT] = {
    [LOG_PRINT] = { true, NULL, NULL, NULL },
    [LOG_TRACE] = { true, "trace.txt", "GEN_TRACE", NULL }
};

static inline bool get_bool_env(const char *key)
{
    const char *val = getenv(key);
    return (val != NULL && strcmp(val, "1") == 0);
}

__attribute__((constructor)) void dbg_log_constructor()
{
    for (int i = 0; i < LOG_MOD_COUNT; i++) {
        log_module_attr_t *mod = g_log_mod_attrs + i;
        if (mod->path != NULL && get_bool_env(mod->env_switch)) {
            mod->fp = fopen(mod->path, "w");
        }
    }
}

__attribute__((destructor)) void dbg_log_destructor()
{
    for (int i = 0; i < LOG_MOD_COUNT; i++) {
        log_module_attr_t *mod = g_log_mod_attrs + i;
        if (mod->fp != NULL) {
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