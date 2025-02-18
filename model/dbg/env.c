#include "env.h"
#include <string.h>
#include "base/types.h"

bool dbg_get_bool_env(const char *key)
{
    if (key == NULL) {
        return false;
    }

    const char *val = getenv(key);
    return (val != NULL && strcmp(val, "0") != 0);
}