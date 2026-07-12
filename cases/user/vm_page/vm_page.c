#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int fail(const char *name)
{
    printf("vm_page: FAIL %s\n", name);
    return 1;
}

static uint8_t pat(uint32_t idx)
{
    return (uint8_t)((idx * 37u + 11u) & 0xffu);
}

int main(void)
{
    long page_size_long = sysconf(_SC_PAGESIZE);
    if (page_size_long <= 0) {
        return fail("pagesize");
    }

    size_t page_size = (size_t)page_size_long;
    size_t map_size = page_size * 2u;
    uint8_t *mem = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return fail("mmap");
    }

    for (size_t i = 0; i < map_size; i++) {
        mem[i] = pat((uint32_t)i);
    }

    uint32_t sum = 0;
    uint32_t exp = 0;
    for (size_t i = page_size - 32u; i < page_size + 32u; i++) {
        sum += mem[i];
        exp += pat((uint32_t)i);
    }
    if (sum != exp) {
        munmap(mem, map_size);
        return fail("cross_page_sum");
    }

    if (mprotect(mem + page_size, page_size, PROT_READ) != 0) {
        munmap(mem, map_size);
        return fail("mprotect_read");
    }

    volatile uint8_t read_back = mem[page_size + 17u];
    if (read_back != pat((uint32_t)(page_size + 17u))) {
        munmap(mem, map_size);
        return fail("mprotect_readback");
    }

    if (mprotect(mem + page_size, page_size, PROT_READ | PROT_WRITE) != 0) {
        munmap(mem, map_size);
        return fail("mprotect_write");
    }

    mem[page_size + 17u] = 0x5au;
    if (mem[page_size + 17u] != 0x5au) {
        munmap(mem, map_size);
        return fail("post_mprotect_write");
    }

    if (munmap(mem, map_size) != 0) {
        return fail("munmap");
    }

    puts("vm_page: PASS");
    return 0;
}

