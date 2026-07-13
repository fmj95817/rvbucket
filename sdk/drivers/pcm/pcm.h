#ifndef PCM_H
#define PCM_H

#include <stdint.h>
#include "drivers/pcm/pcm_map.h"

#define PCM_BASE_ADDR 0x30003000u

extern void pcm_clear(void);
extern uint64_t pcm_read(uint32_t idx);

#endif
