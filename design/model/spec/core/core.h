#ifndef CONF_CORE_H
#define CONF_CORE_H

#include "base/def.h"

#define HART_NUM       1u
#define L2_SIZE        (128u * KiB)
#define L2_WAY_NUM     4u
#define L2_LATENCY     1u

#define CORE_BUS_STG_FIFO_DEPTH 16u
#define CORE_BUS_OST_DEPTH 8u
#define CORE_L2_STG_FIFO_DEPTH 16u
#define CORE_L2_BYPASS_OST_DEPTH 4u

#define BOOT_ROM_BASE  0x00000000u
#define BOOT_ROM_SIZE  (4u * KiB)
#define MM_BASE        0x40000000u
#define MM_SIZE        (2u * GiB)
#define CFG_BASE       0x30000000u
#define CFG_SIZE       (32u * MiB)

#define PERI_BASE      0x30000000u
#define PERI_SIZE      (1u * MiB)
#define IO_SUBSYS_BASE 0x30100000u
#define IO_SUBSYS_SIZE (1u * MiB)
#define ACLINT_BASE    0x31000000u
#define ACLINT_SIZE    (1u * MiB)
#define PLIC_BASE      0x31100000u
#define PLIC_SIZE      (4u * MiB)

#define ACLINT_MTIMER_BASE        0x31000000u
#define ACLINT_MTIMER_SIZE        0x8u
#define ACLINT_MTIMECMP_BASE      0x31010000u
#define ACLINT_MTIMECMP_SIZE      0x7ff8u
#define ACLINT_MTIMER_TICK_CYCLES 10000u
#define ACLINT_MSWI_BASE          0x31020000u
#define ACLINT_MSWI_SIZE          0x4000u
#define ACLINT_SSWI_BASE          0x31030000u
#define ACLINT_SSWI_SIZE          0x4000u

#define PLIC_MAX_IRQ_NUM     32u
#define PLIC_SOURCE_NUM      (PLIC_MAX_IRQ_NUM + 1u)
#define PLIC_BITFIELD_WORDS  ((PLIC_SOURCE_NUM + 31u) / 32u)

#endif
