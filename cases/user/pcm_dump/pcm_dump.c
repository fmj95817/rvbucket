#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "pcm_map.h"

#define PCM_BASE_ADDR 0x30003000u
#define PCM_MAP_SIZE  0x1000u

static int ensure_dev_mem(void)
{
    if (access("/dev/mem", R_OK | W_OK) == 0)
        return 0;

    if (errno != ENOENT)
        return -1;

    if (mkdir("/dev", 0755) != 0 && errno != EEXIST)
        return -1;

    if (mknod("/dev/mem", S_IFCHR | 0600, makedev(1, 1)) != 0 &&
        errno != EEXIST)
        return -1;

    return 0;
}

static uint64_t read_counter(volatile uint32_t *regs, uint32_t idx)
{
    uint32_t off = idx * 2;
    uint32_t hi0;
    uint32_t lo;
    uint32_t hi1;

    do {
        hi0 = regs[off + 1];
        lo = regs[off];
        hi1 = regs[off + 1];
    } while (hi0 != hi1);

    return ((uint64_t)hi1 << 32) | lo;
}

int main(int argc, char **argv)
{
    int show_all = 0;
    int clear = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "clear") == 0 || strcmp(argv[i], "--clear") == 0)
            clear = 1;
        else if (strcmp(argv[i], "--all") == 0)
            show_all = 1;
        else {
            fprintf(stderr, "usage: %s [--all] [clear]\n", argv[0]);
            return 1;
        }
    }

    if (ensure_dev_mem() != 0) {
        perror("pcm_dump: /dev/mem");
        return 1;
    }

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("pcm_dump: open /dev/mem");
        return 1;
    }

    volatile uint32_t *regs = mmap(NULL, PCM_MAP_SIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, PCM_BASE_ADDR);
    if (regs == MAP_FAILED) {
        perror("pcm_dump: mmap");
        close(fd);
        return 1;
    }

    if (clear) {
        regs[PCM_REG_CLEAR / 4] = 1;
        puts("pcm_dump: cleared");
    } else {
        for (uint32_t i = 0; i < PCM_COUNTER_NUM; i++) {
            const char *name = pcm_counter_name(i);
            uint64_t value;
            if (!show_all && name == NULL)
                continue;

            value = read_counter(regs, i);
            if (show_all || value != 0)
                printf("%03u %-28s %llu\n", i, name ? name : "-",
                    (unsigned long long)value);
        }
    }

    munmap((void *)regs, PCM_MAP_SIZE);
    close(fd);
    return 0;
}
