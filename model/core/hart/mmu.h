#ifndef MMU_H
#define MMU_H

#include "base/types.h"
#include "base/itf.h"

typedef struct mmu_conf {
} mmu_conf_t;

typedef struct mmu {
} mmu_t;

extern void mmu_construct(mmu_t *mmu, const mmu_conf_t *conf);
extern void mmu_reset(mmu_t *mmu);
extern void mmu_clock(mmu_t *mmu);
extern void mmu_free(mmu_t *mmu);


#endif