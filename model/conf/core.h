#ifndef CONF_CORE_H
#define CONF_CORE_H

#include "base/def.h"

#define BOOT_ROM_BASE  0x00000000
#define BOOT_ROM_SIZE  (1u * KiB)
#define ITCM_BASE      0x10000000
#define ITCM_SIZE      (128u * KiB)
#define DTCM_BASE      0x20000000
#define DTCM_SIZE      (64u * KiB)
#define MM_BASE        0x40000000
#define MM_SIZE        (2u * GiB)
#define CFG_BASE       0x30000000
#define CFG_SIZE       (32u * MiB)

#define PERI_BASE      0x30000000
#define PERI_SIZE      (16u * MiB)
#define ACLINT_BASE    0x31000000
#define ACLINT_SIZE    (1u * MiB)
#define PLIC_BASE      0x31100000
#define PLIC_SIZE      (1u * MiB)

#define ACLINT_MTIMER_BASE    0x31000000
#define ACLINT_MTIMER_SIZE    0x8
#define ACLINT_MTIMECMP_BASE  0x31010000
#define ACLINT_MTIMECMP_SIZE  0x7ff8
#define ACLINT_MSWI_BASE      0x31020000
#define ACLINT_MSWI_SIZE      0x4000
#define ACLINT_SSWI_BASE      0x31030000
#define ACLINT_SSWI_SIZE      0x4000

#define PLIC_MAX_IRQ_NUM 32

#endif