#include "ui_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define BUF_SIZE 256

typedef struct {
    sim_ui_t ui;
    struct termios orig_termios;
    bool tty_raw;
    char buf[BUF_SIZE];
    u32 rd, wr;
} ui_term_t;

static void uart_out(void *ctx, u8 ch)
{
    (void)ctx;
    putchar((int)ch);
    fflush(stdout);
}

static bool uart_in(void *ctx, u8 *ch)
{
    ui_term_t *ut = (ui_term_t *)ctx;

    while (1) {
        u32 nw = (ut->wr + 1) % BUF_SIZE;
        if (nw == ut->rd) {
            break;
        }
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            break;
        }
        ut->buf[ut->wr] = c;
        ut->wr = nw;
    }
    if (ut->rd == ut->wr) {
        return false;
    }
    *ch = (u8)ut->buf[ut->rd];
    ut->rd = (ut->rd + 1) % BUF_SIZE;
    return true;
}

static void gpio_change(void *ctx, u32 val)
{
    (void)ctx;
    (void)val;
}

static void cleanup(void *ctx)
{
    ui_term_t *ut = (ui_term_t *)ctx;
    int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, fl & ~O_NONBLOCK);
    if (ut->tty_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &ut->orig_termios);
    }
    free(ut);
}

sim_ui_t *ui_term_create(void)
{
    ui_term_t *ut = calloc(1, sizeof(ui_term_t));
    ut->ui.uart_out = uart_out;
    ut->ui.uart_in = uart_in;
    ut->ui.gpio_change = gpio_change;
    ut->ui.cleanup = cleanup;

    if (isatty(STDIN_FILENO)) {
        tcgetattr(STDIN_FILENO, &ut->orig_termios);
        struct termios raw = ut->orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        ut->tty_raw = true;
    }
    int fl = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, fl | O_NONBLOCK);

    return &ut->ui;
}
