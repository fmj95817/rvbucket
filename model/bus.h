#ifndef BUS_H
#define BUS_H

#include "types.h"

typedef enum bus_cmd {
    BUS_CMD_READ = 0,
    BUS_CMD_WRITE = 1
} bus_cmd_t;

typedef struct bus_req {
    bus_cmd_t cmd;
    u32 addr;
    u32 data;
    u8 strobe;
} bus_req_t;

typedef struct bus_rsp {
    u32 data;
    bool ok;
} bus_rsp_t;

typedef bus_rsp_t (*bus_req_handler_t)(void *dev, const bus_req_t *req);

typedef struct bus_if {
    void *dev;
    bus_req_handler_t req_handler;
} bus_if_t;

#endif