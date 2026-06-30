#include "env.h"
#include <string.h>
#include <stdio.h>
#include "base/types.h"

bool dbg_get_bool_env(const char *key)
{
    if (key == NULL) {
        return false;
    }

    const char *buf = getenv(key);
    return (buf != NULL && strcmp(buf, "0") != 0);
}

u32 dbg_get_uint_env(const char *key)
{
    if (key == NULL) {
        return 0;
    }

    const char *buf = getenv(key);
    if (buf == NULL) {
        return 0;
    }

    u32 val = 0;
    sscanf(buf, "%u", &val);

    return val;
}