#include "soc.h"
#include "bti.h"

#define BOOT_ROM_BASE_ADDR  0x40000000
#define BOOT_ROM_SIZE       (1 * KiB)

#define FLASH_BASE_ADDR     0x80000000
#define FLASH_SIZE          (1 * MiB)

#define ITCM_BASE_ADDR      0x10000000
#define ITCM_SIZE           (128 * KiB)

#define DTCM_BASE_ADDR      0x20000000
#define DTCM_SIZE           (64 * KiB)

#define UART_BASE_ADDR      0x30000000
#define UART_SIZE           4

void soc_construct(soc_t *soc, const u64 *cycle)
{
    extern u32 g_boot_code_size;
    extern u8 g_boot_code[];

    const u32 i_periph_base_addrs[] = { BOOT_ROM_BASE_ADDR, ITCM_BASE_ADDR };
    const u32 i_periph_sizes[] = { BOOT_ROM_SIZE, ITCM_SIZE };
    const u32 d_periph_base_addrs[] = { FLASH_BASE_ADDR, ITCM_BASE_ADDR, DTCM_BASE_ADDR, UART_BASE_ADDR };
    const u32 d_periph_sizes[] = { FLASH_SIZE, ITCM_SIZE, DTCM_SIZE, UART_SIZE };

    itf_construct(&soc->cpu_i_bti_req_itf, cycle, "cpu_i_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->cpu_i_bti_rsp_itf, cycle, "cpu_i_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->cpu_d_bti_req_itf, cycle, "cpu_d_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->cpu_d_bti_rsp_itf, cycle, "cpu_d_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->boot_rom_bti_req_itf, cycle, "boot_rom_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->boot_rom_bti_rsp_itf, cycle, "boot_rom_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->flash_bti_req_itf, cycle, "flash_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->flash_bti_rsp_itf, cycle, "flash_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->itcm_i_bti_req_itf, cycle, "itcm_i_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->itcm_i_bti_rsp_itf, cycle, "itcm_i_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->itcm_d_bti_req_itf, cycle, "itcm_d_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->itcm_d_bti_rsp_itf, cycle, "itcm_d_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->dtcm_bti_req_itf, cycle, "dtcm_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->dtcm_bti_rsp_itf, cycle, "dtcm_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&soc->uart_bti_req_itf, cycle, "uart_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&soc->uart_bti_rsp_itf, cycle, "uart_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);

    soc->cpu.i_bti_req_mst = &soc->cpu_i_bti_req_itf;
    soc->cpu.i_bti_rsp_slv = &soc->cpu_i_bti_rsp_itf;
    soc->cpu.d_bti_req_mst = &soc->cpu_d_bti_req_itf;
    soc->cpu.d_bti_rsp_slv = &soc->cpu_d_bti_rsp_itf;
    soc->boot_rom.bti_req_slv = &soc->boot_rom_bti_req_itf;
    soc->boot_rom.bti_rsp_mst = &soc->boot_rom_bti_rsp_itf;
    soc->flash.bti_req_slv = &soc->flash_bti_req_itf;
    soc->flash.bti_rsp_mst = &soc->flash_bti_rsp_itf;
    soc->itcm.bti_req_slv[0] = &soc->itcm_i_bti_req_itf;
    soc->itcm.bti_rsp_mst[0] = &soc->itcm_i_bti_rsp_itf;
    soc->itcm.bti_req_slv[1] = &soc->itcm_d_bti_req_itf;
    soc->itcm.bti_rsp_mst[1] = &soc->itcm_d_bti_rsp_itf;
    soc->dtcm.bti_req_slv[0] = &soc->dtcm_bti_req_itf;
    soc->dtcm.bti_rsp_mst[0] = &soc->dtcm_bti_rsp_itf;
    soc->uart.bti_req_slv = &soc->uart_bti_req_itf;
    soc->uart.bti_rsp_mst = &soc->uart_bti_rsp_itf;
    soc->uart.uart_tx = soc->uart_tx;

    soc->i_bti_demux.host_bti_req_slv = &soc->cpu_i_bti_req_itf;
    soc->i_bti_demux.host_bti_rsp_mst = &soc->cpu_i_bti_rsp_itf;
    soc->i_bti_demux.gst_bti_req_msts[0] = &soc->boot_rom_bti_req_itf;
    soc->i_bti_demux.gst_bti_rsp_slvs[0] = &soc->boot_rom_bti_rsp_itf;
    soc->i_bti_demux.gst_bti_req_msts[1] = &soc->itcm_i_bti_req_itf;
    soc->i_bti_demux.gst_bti_rsp_slvs[1] = &soc->itcm_i_bti_rsp_itf;

    soc->d_bti_demux.host_bti_req_slv = &soc->cpu_d_bti_req_itf;
    soc->d_bti_demux.host_bti_rsp_mst = &soc->cpu_d_bti_rsp_itf;
    soc->d_bti_demux.gst_bti_req_msts[0] = &soc->flash_bti_req_itf;
    soc->d_bti_demux.gst_bti_rsp_slvs[0] = &soc->flash_bti_rsp_itf;
    soc->d_bti_demux.gst_bti_req_msts[1] = &soc->itcm_d_bti_req_itf;
    soc->d_bti_demux.gst_bti_rsp_slvs[1] = &soc->itcm_d_bti_rsp_itf;
    soc->d_bti_demux.gst_bti_req_msts[2] = &soc->dtcm_bti_req_itf;
    soc->d_bti_demux.gst_bti_rsp_slvs[2] = &soc->dtcm_bti_rsp_itf;
    soc->d_bti_demux.gst_bti_req_msts[3] = &soc->uart_bti_req_itf;
    soc->d_bti_demux.gst_bti_rsp_slvs[3] = &soc->uart_bti_rsp_itf;

    rv32i_construct(&soc->cpu, cycle, BOOT_ROM_BASE_ADDR, BOOT_ROM_BASE_ADDR, BOOT_ROM_SIZE);
    ram_construct(&soc->itcm, 2, ITCM_SIZE, ITCM_BASE_ADDR);
    ram_construct(&soc->dtcm, 1, DTCM_SIZE, DTCM_BASE_ADDR);
    rom_construct(&soc->flash, FLASH_SIZE, NULL, 0, FLASH_BASE_ADDR);
    rom_construct(&soc->boot_rom, BOOT_ROM_SIZE, g_boot_code, g_boot_code_size, BOOT_ROM_BASE_ADDR);
    uart_construct(&soc->uart, UART_BASE_ADDR);
    bti_demux_construct(&soc->i_bti_demux, 2, i_periph_base_addrs, i_periph_sizes);
    bti_demux_construct(&soc->d_bti_demux, 4, d_periph_base_addrs, d_periph_sizes);
}

void soc_reset(soc_t *soc)
{
    rv32i_reset(&soc->cpu);
    ram_reset(&soc->itcm);
    ram_reset(&soc->dtcm);
    rom_reset(&soc->flash);
    rom_reset(&soc->boot_rom);
    uart_reset(&soc->uart);
    bti_demux_reset(&soc->i_bti_demux);
    bti_demux_reset(&soc->d_bti_demux);
}

void soc_clock(soc_t *soc)
{
    rv32i_clock(&soc->cpu);
    rom_clock(&soc->boot_rom);
    rom_clock(&soc->flash);
    ram_clock(&soc->itcm);
    ram_clock(&soc->dtcm);
    uart_clock(&soc->uart);
    bti_demux_clock(&soc->i_bti_demux);
    bti_demux_clock(&soc->d_bti_demux);
}

void soc_free(soc_t *soc)
{
    rv32i_free(&soc->cpu);
    rom_free(&soc->flash);
    rom_free(&soc->boot_rom);
    ram_free(&soc->itcm);
    ram_free(&soc->dtcm);
    uart_free(&soc->uart);
    bti_demux_free(&soc->i_bti_demux);
    bti_demux_free(&soc->d_bti_demux);

    itf_free(&soc->cpu_i_bti_req_itf);
    itf_free(&soc->cpu_i_bti_rsp_itf);
    itf_free(&soc->cpu_d_bti_req_itf);
    itf_free(&soc->cpu_d_bti_rsp_itf);
    itf_free(&soc->boot_rom_bti_req_itf);
    itf_free(&soc->boot_rom_bti_rsp_itf);
    itf_free(&soc->flash_bti_req_itf);
    itf_free(&soc->flash_bti_rsp_itf);
    itf_free(&soc->itcm_i_bti_req_itf);
    itf_free(&soc->itcm_i_bti_rsp_itf);
    itf_free(&soc->itcm_d_bti_req_itf);
    itf_free(&soc->itcm_d_bti_rsp_itf);
    itf_free(&soc->dtcm_bti_req_itf);
    itf_free(&soc->dtcm_bti_rsp_itf);
    itf_free(&soc->uart_bti_req_itf);
    itf_free(&soc->uart_bti_rsp_itf);
}
