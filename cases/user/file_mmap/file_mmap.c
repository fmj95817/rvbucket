#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int fail(const char *name)
{
    printf("file_mmap: FAIL %s errno=%d\n", name, errno);
    return 1;
}

static uint8_t pat(uint32_t idx)
{
    return (uint8_t)((idx * 19u + 3u) & 0xffu);
}

int main(void)
{
    const char *path = "/tmp/rvbucket_file_mmap.bin";
    const size_t len = 8192u;

    if (mkdir("/tmp", 0777) != 0 && errno != EEXIST) {
        return fail("mkdir");
    }

    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        return fail("open");
    }
    if (ftruncate(fd, (off_t)len) != 0) {
        close(fd);
        return fail("ftruncate");
    }

    uint8_t *map = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        close(fd);
        return fail("mmap");
    }

    for (uint32_t i = 0; i < len; i++) {
        map[i] = pat(i);
    }
    if (msync(map, len, MS_SYNC) != 0) {
        munmap(map, len);
        close(fd);
        return fail("msync");
    }
    if (munmap(map, len) != 0) {
        close(fd);
        return fail("munmap");
    }

    uint8_t buf[64];
    if (pread(fd, buf, sizeof(buf), 4096 - 17) != (ssize_t)sizeof(buf)) {
        close(fd);
        return fail("pread");
    }
    for (uint32_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != pat(4096u - 17u + i)) {
            close(fd);
            return fail("verify");
        }
    }

    if (close(fd) != 0 || unlink(path) != 0) {
        return fail("cleanup");
    }

    puts("file_mmap: PASS");
    return 0;
}

