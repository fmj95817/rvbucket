#ifndef SIM_VIP_SDSPI_VIP_H
#define SIM_VIP_SDSPI_VIP_H

#include <stdio.h>
#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/sdspi_cmd_if.h"
#include "itf/sdspi_data_if.h"
#include "spec/io/sdspi.h"

typedef struct sdspi_vip_conf {
    const char *image_path;
} sdspi_vip_conf_t;

typedef enum sdspi_vip_state {
    SDSPI_VIP_IDLE,
    SDSPI_VIP_SEND_RESPONSE,
    SDSPI_VIP_SEND_READ_DATA,
    SDSPI_VIP_RECV_WRITE_DATA,
    SDSPI_VIP_SEND_WRITE_DONE
} sdspi_vip_state_t;

typedef struct sdspi_vip {
    mod_t mod;
    itf_t *cmd_slv;
    itf_t *data_mst;

    FILE *image;
    u64 image_size;
    bool image_read_only;
    bool card_present;
    bool read_only;
    bool card_idle;
    bool app_cmd;
    bool status_pending;
    bool enabled;
    u32 clock_div;

    sdspi_vip_state_t state;
    sdspi_data_if_t result;
    u8 *data_buf;
    u32 data_len;
    u32 data_offset;
    u64 image_offset;
    bool data_write;
} sdspi_vip_t;

extern void sdspi_vip_construct(sdspi_vip_t *vip, const char *parent,
    const char *name, const sdspi_vip_conf_t *conf);
extern void sdspi_vip_reset(sdspi_vip_t *vip);
extern void sdspi_vip_clock(sdspi_vip_t *vip);
extern void sdspi_vip_set_card_present(sdspi_vip_t *vip, bool present);
extern void sdspi_vip_free(sdspi_vip_t *vip);

#endif
