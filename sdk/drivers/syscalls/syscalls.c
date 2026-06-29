#include <stddef.h>
#include <sys/stat.h>
#include "drivers/uart/uart.h"

int _write(int fd, char *ptr, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) {
        uart_write(ptr[i]);
    }
    return len;
}

int _read(int fd, char *ptr, int len)
{
    (void)fd;
    int i = 0;
    while (i < len) {
        char ch;
        while ((ch = uart_read()) == 0) {
            uart_irq_handler();
        }

        if (ch == 0x7f || ch == '\b') {
            /* backspace: remove last char from buffer */
            if (i > 0) {
                i--;
            }
            continue;
        }

        ptr[i++] = ch;
        if (ch == '\n' || ch == '\r') {
            break;
        }
    }
    return i;
}

int _close(int fd)
{
    (void)fd;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _lseek(int fd, int offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

void *_sbrk(ptrdiff_t incr)
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
