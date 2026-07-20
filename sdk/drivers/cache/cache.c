#include "cache.h"

#include <stdint.h>

#define CACHE_BLOCK_SIZE 64u

static inline void cbo_clean(uintptr_t addr)
{
    __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 1"
        : : "r" (addr) : "memory");
}

static inline void cbo_inval(uintptr_t addr)
{
    __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 0"
        : : "r" (addr) : "memory");
}

static uintptr_t cache_range_end(const void *buffer, uint32_t size)
{
    return ((uintptr_t)buffer + size + CACHE_BLOCK_SIZE - 1u) &
        ~(CACHE_BLOCK_SIZE - 1u);
}

void cache_clean_range(const void *buffer, uint32_t size)
{
    if (size == 0u) {
        return;
    }
    uintptr_t addr = (uintptr_t)buffer & ~(CACHE_BLOCK_SIZE - 1u);
    uintptr_t end = cache_range_end(buffer, size);

    for (; addr < end; addr += CACHE_BLOCK_SIZE) {
        cbo_clean(addr);
    }
    __asm__ volatile ("fence rw, rw" : : : "memory");
}

void cache_invalidate_range(const void *buffer, uint32_t size)
{
    if (size == 0u) {
        return;
    }
    uintptr_t addr = (uintptr_t)buffer & ~(CACHE_BLOCK_SIZE - 1u);
    uintptr_t end = cache_range_end(buffer, size);

    __asm__ volatile ("fence rw, rw" : : : "memory");
    for (; addr < end; addr += CACHE_BLOCK_SIZE) {
        cbo_inval(addr);
    }
    __asm__ volatile ("fence rw, rw" : : : "memory");
}

void cache_sync_instruction_stream(void)
{
    __asm__ volatile ("fence rw, rw\n.word 0x0000100f" : : : "memory");
}
