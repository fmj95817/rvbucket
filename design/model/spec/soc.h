#ifndef CONF_SOC_H
#define CONF_SOC_H

#include "base/def.h"

#define DDR_BASE            0x40000000u
#define DDR_SIZE            (1u * GiB)
#define DDR_LATENCY         50u
#define FLASH_BASE          0x80000000
#define FLASH_SIZE          (32u * MiB)

#define SOC_BUS_STG_FIFO_DEPTH 4u
#define SOC_BUS_OST_DEPTH 4u

#endif
