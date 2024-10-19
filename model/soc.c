#include "soc.h"
#include <stdlib.h>
#include "bus.h"

#define ADDR_IN(addr, base, size) ((addr >= base) && (addr < (base + size)))

#define TCM_BASE_ADDR     0x10000000
#define TCM_SIZE          (128 * KiB)

#define UART_BASE_ADDR    0x20000000
#define UART_SIZE         4

#define FLASH_BASE_ADDR   0x80000000
#define FLASH_SIZE        (1 * MiB)

static bus_rsp_t soc_bus_req_handler(void *dev, const bus_req_t *req)
{
    soc_t *soc = (soc_t*)dev;

    if (ADDR_IN(req->addr, TCM_BASE_ADDR, TCM_SIZE)) {
        return ram_bus_req_handler(&soc->tcm, TCM_BASE_ADDR, req);
    } else if (ADDR_IN(req->addr, UART_BASE_ADDR, UART_SIZE)) {
        return uart_bus_req_handler(&soc->uart, UART_BASE_ADDR, req);
    } else if (ADDR_IN(req->addr, FLASH_BASE_ADDR, FLASH_SIZE)) {
        return rom_bus_req_handler(&soc->flash, FLASH_BASE_ADDR, req);
    } else {
        bus_rsp_t rsp = { .ok = false };
        return rsp;
    }
}

void soc_construct(soc_t *soc, uart_output_t *uart_output, log_sys_t *log_sys)
{
    bus_if_t *soc_bus_if = malloc(sizeof(bus_if_t));
    soc_bus_if->req_handler = &soc_bus_req_handler;
    soc_bus_if->dev = soc;

    rv32i_construct(&soc->cpu, soc_bus_if, TCM_BASE_ADDR, log_sys);
    ram_construct(&soc->tcm, TCM_SIZE);
    rom_construct(&soc->flash, FLASH_SIZE);
    uart_construct(&soc->uart, uart_output);

    soc->log_sys = log_sys;
}

void soc_reset(soc_t *soc)
{
    rv32i_reset(&soc->cpu);
    ram_reset(&soc->tcm);
    rom_reset(&soc->flash);
    uart_reset(&soc->uart);
}

void soc_free(soc_t *soc)
{
    rv32i_free(&soc->cpu);
    rom_free(&soc->flash);
    ram_free(&soc->tcm);
    uart_free(&soc->uart);
}
