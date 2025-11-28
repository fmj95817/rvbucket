#ifndef CONF_SOC_H
#define CONF_SOC_H

#include "base/def.h"

#define DDR_BASE            0x40000000u
#define DDR_SIZE            (1u * GiB)
#define FLASH_BASE          0x80000000
#define FLASH_SIZE          (16u * MiB)

#define UART_BASE           0x30000000u
#define UART_SIZE           12u

#endif