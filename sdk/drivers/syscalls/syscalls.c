#include <stddef.h>
#include <sys/stat.h>
#include "drivers/uart/uart.h"

__attribute__((used)) int _write(int fd, char *ptr, int len)
{
    for (int i = 0; i < len; i++) {
        uart_write(ptr[i]);
    }
    return len;
}

__attribute__((weak)) void *_sbrk(ptrdiff_t incr)
{
    extern char _end[];
    extern char _heap_end[];
    static char *curbrk = _end;

    if ((curbrk + incr < _end) || (curbrk + incr > _heap_end)) {
        return NULL - 1;
    }

    curbrk += incr;
    return curbrk - incr;
}