#include <stdint.h>
#include <stdio.h>

#include "drivers/cache/cache.h"
#include "drivers/sdspi/sdspi.h"

#define DDR_BASE         0x40000000u
#define DDR_ALIAS_BASE   0xc0000000u
#define MMIO_BASE        0x30000000u

#define PTE_V            (1u << 0)
#define PTE_R            (1u << 1)
#define PTE_W            (1u << 2)
#define PTE_X            (1u << 3)
#define PTE_U            (1u << 4)
#define PTE_A            (1u << 6)
#define PTE_D            (1u << 7)

#define SATP_MODE_SV32   (1u << 31)
#define MSTATUS_MPP_MASK (3u << 11)
#define MSTATUS_MPP_S    (1u << 11)

#define TEST_LBA         64u
#define TEST_BLOCKS      128u
#define TEST_ITERATIONS  2u
#define TEST_SIZE        (TEST_BLOCKS * SDSPI_SECTOR_SIZE)
#define HIGH_TEST_LBA    256u
#define HIGH_TEST_BLOCKS 8u
#define HIGH_TEST_SIZE   (HIGH_TEST_BLOCKS * SDSPI_SECTOR_SIZE)
#define HIGH_DMA_PA      0x4f000000u
#define HIGH_ALIAS_BASE  0xd0000000u
#define USER_CODE_VA     0x00177000u
#define USER_DATA_VA     0x00178000u
#define USER_TOHOST_VA   0x00179000u
#define GUARD_SIZE       64u
#define GUARD_VALUE      0x3cu
#define STRESS_SIZE      (256u * 1024u)
#define CACHE_LINE_SIZE  64u
#define STRESS_LINES     (STRESS_SIZE / CACHE_LINE_SIZE)
#define STRESS_INCREMENT 0x01010101u

#define SDSPI_CMD12      12u
#define SDSPI_CMD18      18u
#define SDSPI_CMD25      25u

#define MMIO_WRITE32(addr, val) (*(volatile uint32_t *)(addr) = (val))
#define MMIO_READ32(addr)       (*(volatile uint32_t *)(addr))

typedef struct dma_region {
    uint8_t guard_before[GUARD_SIZE];
    uint8_t data[TEST_SIZE];
    uint8_t guard_after[GUARD_SIZE];
} dma_region_t;

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static uint32_t user_l0_pt[1024] __attribute__((aligned(4096)));
static uint32_t user_code_page[1024] __attribute__((aligned(4096)));
static uint32_t user_data_page[1024] __attribute__((aligned(4096)));
static dma_region_t dma_region __attribute__((aligned(64)));
static uint8_t stress_buffer[STRESS_SIZE] __attribute__((aligned(64)));
static sdspi_dev_t sdspi;

extern uint32_t user_payload_start[];
extern uint32_t user_payload_end[];

uint32_t sdspi_irq_handler(void)
{
    sdspi_irq(&sdspi);
    return 4u;
}

static uint32_t superpage_pte(uint32_t pa, uint32_t flags)
{
    return ((pa >> 12u) << 10u) | flags | PTE_V | PTE_A | PTE_D;
}

static uint8_t pattern(uint32_t iteration, uint32_t offset)
{
    return (uint8_t)((offset * 13u) ^ (iteration * 0x5bu) ^ 0xa5u);
}

static int guards_valid(const uint8_t *alias)
{
    for (uint32_t i = 0; i < GUARD_SIZE; i++) {
        if (alias[i] != GUARD_VALUE ||
            alias[GUARD_SIZE + TEST_SIZE + i] != GUARD_VALUE)
            return 0;
    }
    return 1;
}

static void start_command(uint32_t opcode, uint32_t lba, uint32_t blocks,
    uint8_t *dma_buffer, int write)
{
    uint32_t ctrl = opcode | (SDSPI_RSP_R1 << SDSPI_CMD_CTRL_RSP_SHIFT);

    sdspi.irq_pending = 0;
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_IRQ_STATUS, SDSPI_IRQ_MASK);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CMD_ARG, lba);
    if (blocks != 0u) {
        MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_DMA_ADDR,
            (uintptr_t)dma_buffer);
        MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_BLOCK_SIZE,
            SDSPI_SECTOR_SIZE);
        MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_BLOCK_COUNT, blocks);
        ctrl |= SDSPI_CMD_CTRL_DATA;
        if (write)
            ctrl |= SDSPI_CMD_CTRL_WRITE;
    }
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CMD_CTRL,
        ctrl | SDSPI_CMD_CTRL_START);
}

static int wait_command_with_stress(volatile uint32_t *stress_alias,
    uint32_t *stress_steps, uint32_t expected_irq)
{
    uint32_t timeout = 20000000u;

    while ((sdspi.irq_pending & (expected_irq | SDSPI_IRQ_ERROR)) == 0u) {
        uint32_t line = *stress_steps % STRESS_LINES;

        stress_alias[line * (CACHE_LINE_SIZE / sizeof(uint32_t))] +=
            STRESS_INCREMENT;
        (*stress_steps)++;
        if (--timeout == 0u)
            return -1;
    }

    if ((sdspi.irq_pending & SDSPI_IRQ_ERROR) != 0u ||
        (MMIO_READ32(SDSPI_BASE_ADDR + SDSPI_REG_CMD_STATUS) &
            SDSPI_CMD_STATUS_ERROR) != 0u ||
        (expected_irq == SDSPI_IRQ_DATA_DONE &&
            (MMIO_READ32(SDSPI_BASE_ADDR + SDSPI_REG_DATA_STATUS) &
                SDSPI_DATA_STATUS_ERROR) != 0u))
        return -1;
    return 0;
}

static int stress_valid(volatile uint32_t *stress_alias,
    uint32_t stress_steps)
{
    uint32_t full_sweeps = stress_steps / STRESS_LINES;
    uint32_t remainder = stress_steps % STRESS_LINES;

    cache_clean_range((const void *)stress_alias, STRESS_SIZE);
    cache_invalidate_range((void *)stress_alias, STRESS_SIZE);
    for (uint32_t line = 0; line < STRESS_LINES; line++) {
        uint32_t visits = full_sweeps + (line < remainder ? 1u : 0u);
        uint32_t expected = (0x13570000u ^ line) +
            visits * STRESS_INCREMENT;

        if (stress_alias[line * (CACHE_LINE_SIZE / sizeof(uint32_t))] !=
            expected)
            return 0;
    }
    return 1;
}

static void sim_finish(uint32_t exit_code) __attribute__((noreturn));

static void sim_finish(uint32_t exit_code)
{
    __asm__ volatile (
        "mv t0, %0\n"
        "li t1, 0x30000004\n"
        "li t2, 0x10\n"
        "sw t2, 0(t1)\n"
        "1:\n"
        "wfi\n"
        "j 1b\n"
        :
        : "r" (exit_code)
        : "t0", "t1", "t2", "memory");
    __builtin_unreachable();
}

asm(
    ".section .rodata.user_payload,\"a\",@progbits\n"
    ".align 2\n"
    ".globl user_payload_start\n"
    "user_payload_start:\n"
    "    li t0, 0x00178000\n"
    "    lw a0, 0x194(t0)\n"
    "    li t1, 0xb88d8297\n"
    "    bne a0, t1, 1f\n"
    "    li t0, 0\n"
    "    j 2f\n"
    "1:\n"
    "    li t0, 16\n"
    "2:\n"
    "    li t1, 0x00179004\n"
    "    li t2, 0x10\n"
    "    sw t2, 0(t1)\n"
    "3:\n"
    "    wfi\n"
    "    j 3b\n"
    ".globl user_payload_end\n"
    "user_payload_end:\n"
    ".section .text\n"
);

static void enter_user_copy_check(const uint8_t *source)
    __attribute__((noreturn));

static void enter_user_copy_check(const uint8_t *source)
{
    uintptr_t data_offset = (uintptr_t)user_data_page - DDR_BASE;
    volatile const uint32_t *source_words =
        (volatile const uint32_t *)source;
    volatile uint32_t *data_alias =
        (volatile uint32_t *)(DDR_ALIAS_BASE + data_offset);
    uint32_t payload_words = (uint32_t)(user_payload_end -
        user_payload_start);

    for (uint32_t i = 0; i < 1024u; i++)
        data_alias[i] = 0x00000023u;
    for (uint32_t i = 0; i < 1024u; i++)
        data_alias[i] = source_words[i];
    for (uint32_t i = 0; i < payload_words; i++)
        user_code_page[i] = user_payload_start[i];

    user_l0_pt[(USER_CODE_VA >> 12u) & 0x3ffu] =
        superpage_pte((uintptr_t)user_code_page, PTE_R | PTE_X | PTE_U);
    user_l0_pt[(USER_DATA_VA >> 12u) & 0x3ffu] =
        superpage_pte((uintptr_t)user_data_page, PTE_R | PTE_W | PTE_U);
    user_l0_pt[(USER_TOHOST_VA >> 12u) & 0x3ffu] =
        superpage_pte(MMIO_BASE, PTE_R | PTE_W | PTE_U);
    root_pt[USER_CODE_VA >> 22u] =
        (((uintptr_t)user_l0_pt >> 12u) << 10u) | PTE_V;

    printf("sdspi_sv32_dma: enter user-copy check\n");
    __asm__ volatile (
        ".word 0x0000100f\n"
        "sfence.vma\n"
        "csrci sstatus, 2\n"
        "li t0, 0x100\n"
        "csrc sstatus, t0\n"
        "li t0, 0x00177000\n"
        "csrw sepc, t0\n"
        "sret\n"
        :
        :
        : "t0", "memory");
    __builtin_unreachable();
}

static void s_mode_entry(void) __attribute__((noreturn));

static void s_mode_entry(void)
{
    uintptr_t region_offset = (uintptr_t)&dma_region - DDR_BASE;
    uintptr_t stress_offset = (uintptr_t)stress_buffer - DDR_BASE;
    uint8_t *region_alias = (uint8_t *)(DDR_ALIAS_BASE + region_offset);
    uint8_t *alias = region_alias + GUARD_SIZE;
    uint8_t *dma_buffer = dma_region.data;
    volatile uint32_t *stress_alias =
        (volatile uint32_t *)(DDR_ALIAS_BASE + stress_offset);
    uint32_t stress_steps = 0;

    if (sdspi_init(&sdspi, SDSPI_WAIT_IRQ) != 0) {
        printf("sdspi_sv32_dma: init failed\n");
        sim_finish(1);
    }

    for (uint32_t i = 0; i < GUARD_SIZE; i++) {
        region_alias[i] = GUARD_VALUE;
        region_alias[GUARD_SIZE + TEST_SIZE + i] = GUARD_VALUE;
    }
    cache_clean_range(region_alias, sizeof(dma_region));

    for (uint32_t iteration = 0; iteration < TEST_ITERATIONS; iteration++) {
        for (uint32_t i = 0; i < TEST_SIZE; i++)
            alias[i] = pattern(iteration, i);

        cache_clean_range(alias, TEST_SIZE);
        if (sdspi_write_blocks(&sdspi, TEST_LBA, TEST_BLOCKS,
            dma_buffer) != 0) {
            printf("sdspi_sv32_dma: write %lu failed\n",
                (unsigned long)iteration);
            sim_finish(2);
        }

        for (uint32_t i = 0; i < TEST_SIZE; i++)
            alias[i] = 0;

        if (sdspi_read_blocks_raw(&sdspi, TEST_LBA, TEST_BLOCKS,
            dma_buffer) != 0) {
            printf("sdspi_sv32_dma: read %lu failed\n",
                (unsigned long)iteration);
            sim_finish(3);
        }
        cache_invalidate_range(alias, TEST_SIZE);
        cache_invalidate_range(region_alias, sizeof(dma_region));

        if (!guards_valid(region_alias)) {
            printf("sdspi_sv32_dma: guard corrupted iter=%lu\n",
                (unsigned long)iteration);
            sim_finish(4);
        }

        for (uint32_t i = 0; i < TEST_SIZE; i++) {
            if (alias[i] != pattern(iteration, i)) {
                printf("sdspi_sv32_dma: mismatch iter=%lu offset=%lu\n",
                    (unsigned long)iteration, (unsigned long)i);
                sim_finish(5);
            }
        }
    }

    for (uint32_t line = 0; line < STRESS_LINES; line++)
        stress_alias[line * (CACHE_LINE_SIZE / sizeof(uint32_t))] =
            0x13570000u ^ line;
    cache_clean_range((const void *)stress_alias, STRESS_SIZE);
    cache_invalidate_range((void *)stress_alias, STRESS_SIZE);

    for (uint32_t i = 0; i < TEST_SIZE; i++)
        alias[i] = pattern(TEST_ITERATIONS, i);
    cache_clean_range(alias, TEST_SIZE);
    start_command(SDSPI_CMD25, TEST_LBA, TEST_BLOCKS, dma_buffer, 1);
    if (wait_command_with_stress(stress_alias, &stress_steps,
        SDSPI_IRQ_DATA_DONE) != 0) {
        printf("sdspi_sv32_dma: concurrent write failed\n");
        sim_finish(6);
    }

    for (uint32_t i = 0; i < TEST_SIZE; i++)
        alias[i] = 0;
    cache_clean_range(alias, TEST_SIZE);
    start_command(SDSPI_CMD18, TEST_LBA, TEST_BLOCKS, dma_buffer, 0);
    if (wait_command_with_stress(stress_alias, &stress_steps,
        SDSPI_IRQ_DATA_DONE) != 0) {
        printf("sdspi_sv32_dma: concurrent read failed\n");
        sim_finish(7);
    }
    start_command(SDSPI_CMD12, 0u, 0u, dma_buffer, 0);
    if (wait_command_with_stress(stress_alias, &stress_steps,
        SDSPI_IRQ_CMD_DONE) != 0) {
        printf("sdspi_sv32_dma: stop failed\n");
        sim_finish(8);
    }

    cache_invalidate_range(region_alias, sizeof(dma_region));
    if (!guards_valid(region_alias)) {
        printf("sdspi_sv32_dma: concurrent guard corrupted\n");
        sim_finish(9);
    }
    for (uint32_t i = 0; i < TEST_SIZE; i++) {
        if (alias[i] != pattern(TEST_ITERATIONS, i)) {
            printf("sdspi_sv32_dma: concurrent mismatch offset=%lu\n",
                (unsigned long)i);
            sim_finish(10);
        }
    }
    if (!stress_valid(stress_alias, stress_steps)) {
        printf("sdspi_sv32_dma: concurrent CPU traffic corrupted\n");
        sim_finish(11);
    }

    uint8_t *high_alias = (uint8_t *)HIGH_ALIAS_BASE;
    uint8_t *high_dma = (uint8_t *)HIGH_DMA_PA;
    for (uint32_t i = 0; i < HIGH_TEST_SIZE; i++)
        high_alias[i] = pattern(7u, i);
    cache_clean_range(high_alias, HIGH_TEST_SIZE);
    start_command(SDSPI_CMD25, HIGH_TEST_LBA, HIGH_TEST_BLOCKS, high_dma, 1);
    if (wait_command_with_stress(stress_alias, &stress_steps,
        SDSPI_IRQ_DATA_DONE) != 0) {
        printf("sdspi_sv32_dma: high-PA write failed\n");
        sim_finish(12);
    }

    for (uint32_t i = 0; i < HIGH_TEST_SIZE; i++)
        high_alias[i] = 0;
    cache_clean_range(high_alias, HIGH_TEST_SIZE);
    start_command(SDSPI_CMD18, HIGH_TEST_LBA, HIGH_TEST_BLOCKS, high_dma, 0);
    if (wait_command_with_stress(stress_alias, &stress_steps,
        SDSPI_IRQ_DATA_DONE) != 0) {
        printf("sdspi_sv32_dma: high-PA read failed\n");
        sim_finish(13);
    }
    start_command(SDSPI_CMD12, 0u, 0u, high_dma, 0);
    if (wait_command_with_stress(stress_alias, &stress_steps,
        SDSPI_IRQ_CMD_DONE) != 0) {
        printf("sdspi_sv32_dma: high-PA stop failed\n");
        sim_finish(14);
    }
    cache_invalidate_range(high_alias, HIGH_TEST_SIZE);
    for (uint32_t i = 0; i < HIGH_TEST_SIZE; i++) {
        if (high_alias[i] != pattern(7u, i)) {
            printf("sdspi_sv32_dma: high-PA mismatch offset=%lu\n",
                (unsigned long)i);
            sim_finish(15);
        }
    }

    enter_user_copy_check(alias);
}

int main(void)
{
    root_pt[DDR_BASE >> 22u] =
        superpage_pte(DDR_BASE, PTE_R | PTE_W | PTE_X);
    root_pt[DDR_ALIAS_BASE >> 22u] =
        superpage_pte(DDR_BASE, PTE_R | PTE_W);
    root_pt[HIGH_ALIAS_BASE >> 22u] =
        superpage_pte(HIGH_DMA_PA, PTE_R | PTE_W);
    root_pt[MMIO_BASE >> 22u] =
        superpage_pte(MMIO_BASE, PTE_R | PTE_W);

    uint32_t satp = SATP_MODE_SV32 | ((uint32_t)root_pt >> 12u);
    __asm__ volatile ("csrw satp, %0\nsfence.vma" : : "r" (satp) : "memory");

    uint32_t mstatus;
    __asm__ volatile ("csrr %0, mstatus" : "=r" (mstatus));
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    __asm__ volatile ("csrw mstatus, %0" : : "r" (mstatus) : "memory");
    __asm__ volatile ("csrw mepc, %0" : : "r" (s_mode_entry) : "memory");
    __asm__ volatile ("mret" : : : "memory");

    __builtin_unreachable();
}
