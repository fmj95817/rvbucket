#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static int initialized;
static int stdin_eof;
static int old_flags;
static int have_old_flags;
static int have_old_termios;
static struct termios old_termios;

static void uart_stdin_restore(void)
{
    if (have_old_termios) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    }
    if (have_old_flags) {
        fcntl(STDIN_FILENO, F_SETFL, old_flags);
    }
}

static void uart_stdin_init(void)
{
    if (initialized) {
        return;
    }
    initialized = 1;

    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &old_termios) == 0) {
        struct termios term = old_termios;
        term.c_lflag &= ~(ICANON | ECHO);
        term.c_cc[VMIN] = 0;
        term.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == 0) {
            have_old_termios = 1;
        }
    }

    old_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (old_flags >= 0 && fcntl(STDIN_FILENO, F_SETFL,
        old_flags | O_NONBLOCK) == 0) {
        have_old_flags = 1;
    }

    atexit(uart_stdin_restore);
}

#ifdef __cplusplus
extern "C" {
#endif

int uart_stdin_read(void)
{
    unsigned char ch;
    ssize_t ret;

    uart_stdin_init();

    if (stdin_eof) {
        return -1;
    }

    ret = read(STDIN_FILENO, &ch, 1);
    if (ret == 1) {
        return ch;
    }
    if (ret == 0) {
        if (!isatty(STDIN_FILENO)) {
            stdin_eof = 1;
        }
        return -1;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return -1;
    }
    return -1;
}

#ifdef __cplusplus
}
#endif
