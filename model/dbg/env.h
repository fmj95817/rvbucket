#ifndef DBG_ENV_H
#define DBG_ENV_H

#include "base/types.h"

extern bool dbg_get_bool_env(const char *key);
extern u32 dbg_get_uint_env(const char *key);

#endif