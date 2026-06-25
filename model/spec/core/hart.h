#ifndef CONF_HART_H
#define CONF_HART_H

#include "base/def.h"

#define IFU_BHT_SIZE 64u
#define MMU_ITLB_SIZE 64u
#define MMU_DTLB_SIZE 32u
#define FCH_TRANS_ID 0u
#define LDST_TRANS_ID 1u
#define L1I_SIZE (16u * KiB)
#define L1I_WAY_NUM 2u
#define L1D_SIZE (16u * KiB)
#define L1D_WAY_NUM 2u

#endif
