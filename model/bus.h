#ifndef BUS_H
#define BUS_H

#include "types.h"

typedef enum bus_cmd {
    BUS_CMD_READ = 0,
    BUS_CMD_WRITE = 1
} bus_cmd_t;

typedef struct bus_trans_if {
    struct {
        bus_cmd_t cmd;
        u32 addr;
        u32 data;
        u8 strobe;
    } req;
    struct {
        u32 data;
        bool ok;
    } rsp;
} bus_trans_if_t;

typedef void (*bus_trans_handler_t)(void *dev, bus_trans_if_t *i);

typedef struct bus_if {
    void *dev;
    bus_trans_handler_t trans_handler;
} bus_if_t;

#endif