#include "soc.h"
#include "bus.h"

#define TCM_BASE_ADDR       0x10000000
#define TCM_SIZE            (128 * KiB)

#define UART_BASE_ADDR      0x20000000
#define UART_SIZE           4

#define BOOT_ROM_BASE_ADDR  0x40000000
#define BOOT_ROM_SIZE       (1 * KiB)

#define FLASH_BASE_ADDR     0x80000000
#define FLASH_SIZE          (1 * MiB)

static void soc_bus_req_handler(void *dev, bus_trans_if_t *i)
{
    soc_t *soc = (soc_t*)dev;

    if (ADDR_IN(i->req.addr, TCM_BASE_ADDR, TCM_SIZE)) {
        ram_bus_trans_handler(&soc->tcm, TCM_BASE_ADDR, i);
    } else if (ADDR_IN(i->req.addr, UART_BASE_ADDR, UART_SIZE)) {
        uart_bus_trans_handler(&soc->uart, UART_BASE_ADDR, i);
    } else if (ADDR_IN(i->req.addr, BOOT_ROM_BASE_ADDR, BOOT_ROM_SIZE)) {
        rom_bus_trans_handler(&soc->boot_rom, BOOT_ROM_BASE_ADDR, i);
    } else if (ADDR_IN(i->req.addr, FLASH_BASE_ADDR, FLASH_SIZE)) {
        rom_bus_trans_handler(&soc->flash, FLASH_BASE_ADDR, i);
    } else {
        i->rsp.ok = false;
    }
}

void soc_construct(soc_t *soc, uart_output_t *uart_output)
{
    bus_if_t *soc_bus_if = malloc(sizeof(bus_if_t));
    soc_bus_if->trans_handler = &soc_bus_req_handler;
    soc_bus_if->dev = soc;

    extern u32 g_boot_code_size;
    extern u8 g_boot_code[];

    rv32i_construct(&soc->cpu, soc_bus_if, BOOT_ROM_BASE_ADDR, BOOT_ROM_BASE_ADDR, BOOT_ROM_SIZE);
    ram_construct(&soc->tcm, TCM_SIZE);
    rom_construct(&soc->flash, FLASH_SIZE, NULL, 0);
    rom_construct(&soc->boot_rom, BOOT_ROM_SIZE, g_boot_code, g_boot_code_size);
    uart_construct(&soc->uart, uart_output);
}

void soc_reset(soc_t *soc)
{
    rv32i_reset(&soc->cpu);
    ram_reset(&soc->tcm);
    rom_reset(&soc->flash);
    rom_reset(&soc->boot_rom);
    uart_reset(&soc->uart);
}

void soc_free(soc_t *soc)
{
    rv32i_free(&soc->cpu);
    rom_free(&soc->flash);
    rom_free(&soc->boot_rom);
    ram_free(&soc->tcm);
    uart_free(&soc->uart);
}
