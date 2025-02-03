#include "soc.h"
#include "bti.h"

#define BOOT_ROM_BASE_ADDR  0x40000000
#define BOOT_ROM_SIZE       (1 * KiB)

#define FLASH_BASE_ADDR     0x80000000
#define FLASH_SIZE          (1 * MiB)

#define TCM_BASE_ADDR       0x10000000
#define TCM_SIZE            (128 * KiB)

#define UART_BASE_ADDR      0x20000000
#define UART_SIZE           4

void soc_construct(soc_t *soc, u64 *cycles)
{
    extern u32 g_boot_code_size;
    extern u8 g_boot_code[];

    const u32 periph_base_addrs[] = { BOOT_ROM_BASE_ADDR, FLASH_BASE_ADDR, TCM_BASE_ADDR, UART_BASE_ADDR };
    const u32 periph_sizes[] = { BOOT_ROM_SIZE, FLASH_SIZE, TCM_SIZE, UART_SIZE };

    itf_construct(&soc->cpu_bti_req_itf, cycles, "cpu_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->cpu_bti_rsp_itf, cycles, "cpu_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->boot_rom_bti_req_itf, cycles, "boot_rom_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->boot_rom_bti_rsp_itf, cycles, "boot_rom_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->flash_bti_req_itf, cycles, "flash_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->flash_bti_rsp_itf, cycles, "flash_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->tcm_bti_req_itf, cycles, "tcm_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->tcm_bti_rsp_itf, cycles, "tcm_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->uart_bti_req_itf, cycles, "uart_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->uart_bti_rsp_itf, cycles, "uart_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);

    soc->cpu.bti_req_mst = &soc->cpu_bti_req_itf;
    soc->cpu.bti_rsp_slv = &soc->cpu_bti_rsp_itf;
    soc->boot_rom.bti_req_slv = &soc->boot_rom_bti_req_itf;
    soc->boot_rom.bti_rsp_mst = &soc->boot_rom_bti_rsp_itf;
    soc->flash.bti_req_slv = &soc->flash_bti_req_itf;
    soc->flash.bti_rsp_mst = &soc->flash_bti_rsp_itf;
    soc->tcm.bti_req_slv = &soc->tcm_bti_req_itf;
    soc->tcm.bti_rsp_mst = &soc->tcm_bti_rsp_itf;
    soc->uart.bti_req_slv = &soc->uart_bti_req_itf;
    soc->uart.bti_rsp_mst = &soc->uart_bti_rsp_itf;
    soc->uart.uart_mst = soc->uart_mst;

    soc->bti_mux.host_bti_req_slv = &soc->cpu_bti_req_itf;
    soc->bti_mux.host_bti_rsp_mst = &soc->cpu_bti_rsp_itf;

    rv32i_construct(&soc->cpu, cycles, BOOT_ROM_BASE_ADDR, BOOT_ROM_BASE_ADDR, BOOT_ROM_SIZE);
    ram_construct(&soc->tcm, TCM_SIZE, TCM_BASE_ADDR);
    rom_construct(&soc->flash, FLASH_SIZE, NULL, 0, FLASH_BASE_ADDR);
    rom_construct(&soc->boot_rom, BOOT_ROM_SIZE, g_boot_code, g_boot_code_size, BOOT_ROM_BASE_ADDR);
    uart_construct(&soc->uart, UART_BASE_ADDR);
    bti_mux_construct(&soc->bti_mux, 4, periph_base_addrs, periph_sizes);

    soc->bti_mux.gst_bti_req_msts[0] = &soc->boot_rom_bti_req_itf;
    soc->bti_mux.gst_bti_rsp_slvs[0] = &soc->boot_rom_bti_rsp_itf;
    soc->bti_mux.gst_bti_req_msts[1] = &soc->flash_bti_req_itf;
    soc->bti_mux.gst_bti_rsp_slvs[1] = &soc->flash_bti_rsp_itf;
    soc->bti_mux.gst_bti_req_msts[2] = &soc->tcm_bti_req_itf;
    soc->bti_mux.gst_bti_rsp_slvs[2] = &soc->tcm_bti_rsp_itf;
    soc->bti_mux.gst_bti_req_msts[3] = &soc->uart_bti_req_itf;
    soc->bti_mux.gst_bti_rsp_slvs[3] = &soc->uart_bti_rsp_itf;
}

void soc_reset(soc_t *soc)
{
    rv32i_reset(&soc->cpu);
    ram_reset(&soc->tcm);
    rom_reset(&soc->flash);
    rom_reset(&soc->boot_rom);
    uart_reset(&soc->uart);
    bti_mux_reset(&soc->bti_mux);
}

void soc_clock(soc_t *soc)
{
    rv32i_clock(&soc->cpu);
    rom_clock(&soc->boot_rom);
    rom_clock(&soc->flash);
    ram_clock(&soc->tcm);
    uart_clock(&soc->uart);
    bti_mux_clock(&soc->bti_mux);
}

void soc_free(soc_t *soc)
{
    rv32i_free(&soc->cpu);
    rom_free(&soc->flash);
    rom_free(&soc->boot_rom);
    ram_free(&soc->tcm);
    uart_free(&soc->uart);
    bti_mux_free(&soc->bti_mux);

    itf_free(&soc->cpu_bti_req_itf);
    itf_free(&soc->cpu_bti_rsp_itf);
    itf_free(&soc->boot_rom_bti_req_itf);
    itf_free(&soc->boot_rom_bti_rsp_itf);
    itf_free(&soc->flash_bti_req_itf);
    itf_free(&soc->flash_bti_rsp_itf);
    itf_free(&soc->tcm_bti_req_itf);
    itf_free(&soc->tcm_bti_rsp_itf);
    itf_free(&soc->uart_bti_req_itf);
    itf_free(&soc->uart_bti_rsp_itf);
}
