#ifndef DBG_H
#define DBG_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define DBG_CHECK(expr) assert(expr)
#define DBG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif