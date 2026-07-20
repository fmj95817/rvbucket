#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

extern void cache_clean_range(const void *buffer, uint32_t size);
extern void cache_invalidate_range(const void *buffer, uint32_t size);
extern void cache_sync_instruction_stream(void);

#endif
