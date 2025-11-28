#ifndef CONF_CORE_H
#define CONF_CORE_H

#include "base/def.h"

#define HART_NUM       1u

#define BOOT_ROM_BASE  0x00000000u
#define BOOT_ROM_SIZE  (1u * KiB)
#define ITCM_BASE      0x10000000u
#define ITCM_SIZE      (128u * KiB)
#define DTCM_BASE      0x20000000u
#define DTCM_SIZE      (64u * KiB)
#define MM_BASE        0x40000000u
#define MM_SIZE        (2u * GiB)
#define CFG_BASE       0x30000000u
#define CFG_SIZE       (32u * MiB)

#define PERI_BASE      0x30000000u
#define PERI_SIZE      (16u * MiB)
#define ACLINT_BASE    0x31000000u
#define ACLINT_SIZE    (1u * MiB)
#define PLIC_BASE      0x31100000u
#define PLIC_SIZE      (1u * MiB)

#define ACLINT_MTIMER_BASE    0x31000000u
#define ACLINT_MTIMER_SIZE    0x8u
#define ACLINT_MTIMECMP_BASE  0x31010000u
#define ACLINT_MTIMECMP_SIZE  0x7ff8u
#define ACLINT_MSWI_BASE      0x31020000u
#define ACLINT_MSWI_SIZE      0x4000u
#define ACLINT_SSWI_BASE      0x31030000u
#define ACLINT_SSWI_SIZE      0x4000u

#define PLIC_MAX_IRQ_NUM 32u

#endif