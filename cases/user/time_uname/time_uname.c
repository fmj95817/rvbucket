#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

static int fail(const char *name)
{
    printf("time_uname: FAIL %s errno=%d\n", name, errno);
    return 1;
}

int main(void)
{
    struct utsname uts;
    if (uname(&uts) != 0) {
        return fail("uname");
    }
    if (strcmp(uts.machine, "riscv32") != 0) {
        return fail("machine");
    }

    struct timespec mono;
    if (clock_gettime(CLOCK_MONOTONIC, &mono) != 0) {
        return fail("clock_gettime");
    }
    if (mono.tv_nsec < 0 || mono.tv_nsec >= 1000000000L) {
        return fail("clock_range");
    }

    struct tms tms_buf;
    clock_t ticks = times(&tms_buf);
    if (ticks == (clock_t)-1) {
        return fail("times");
    }

    if (getpid() <= 0 || getppid() <= 0) {
        return fail("pid");
    }

    puts("time_uname: PASS");
    return 0;
}

