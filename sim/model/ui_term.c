#include "ui_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>

#define BUF_SIZE 1024
#define CMD_BUF_SIZE 16

typedef struct {
    sim_ui_t ui;
    struct termios orig_termios;
    bool tty_raw;
    bool input_thread_valid;
    bool input_thread_stop;
    bool reset_req;
    bool cmd_mode;
    char cmd_buf[CMD_BUF_SIZE];
    int  cmd_len;
    u32  gpio_in_val;
    bool gpio_in_dirty;
    pthread_t input_thread;
    pthread_mutex_t lock;
    pthread_cond_t space_cond;
    u8 buf[BUF_SIZE];
    u32 rd, wr;
} ui_term_t;

static bool buf_empty(const ui_term_t *ut)
{
    return ut->rd == ut->wr;
}

static bool buf_full(const ui_term_t *ut)
{
    return ((ut->wr + 1u) % BUF_SIZE) == ut->rd;
}

static void *input_thread_main(void *arg)
{
    ui_term_t *ut = (ui_term_t *)arg;

    while (1) {
        u8 ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n == 0) {
            break;
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        pthread_mutex_lock(&ut->lock);
        while (buf_full(ut) && !ut->input_thread_stop) {
            pthread_cond_wait(&ut->space_cond, &ut->lock);
        }
        if (ut->input_thread_stop) {
            pthread_mutex_unlock(&ut->lock);
            break;
        }
        ut->buf[ut->wr] = ch;
        ut->wr = (ut->wr + 1u) % BUF_SIZE;
        pthread_mutex_unlock(&ut->lock);
    }
    return NULL;
}

static void stop_input_thread(ui_term_t *ut)
{
    if (!ut->input_thread_valid) {
        return;
    }

    pthread_mutex_lock(&ut->lock);
    ut->input_thread_stop = true;
    pthread_cond_broadcast(&ut->space_cond);
    pthread_mutex_unlock(&ut->lock);

    pthread_cancel(ut->input_thread);
    pthread_join(ut->input_thread, NULL);
    ut->input_thread_valid = false;
}

static void cmd_prompt(void)
{
    write(STDOUT_FILENO, "\n:", 2);
}

static void cmd_process(ui_term_t *ut)
{
    char *c = ut->cmd_buf;
    int len = ut->cmd_len;
    ut->cmd_len = 0;

    if (len == 1 && c[0] == 'i') {
        /* return to interactive mode */
        ut->cmd_mode = false;
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    if (len == 1 && c[0] == 'r') {
        /* reset — exit cmd mode, set reset request */
        ut->cmd_mode = false;
        ut->reset_req = true;
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    /* p1..p4 — toggle button (GPIO 16..19) */
    if (len == 2 && c[0] == 'p' && c[1] >= '1' && c[1] <= '4') {
        int bit = 16 + (c[1] - '1');
        ut->gpio_in_val ^= (1u << bit);
        ut->gpio_in_dirty = true;
        cmd_prompt();
        return;
    }

    /* s1..s4 — toggle switch (GPIO 20..23) */
    if (len == 2 && c[0] == 's' && c[1] >= '1' && c[1] <= '4') {
        int bit = 20 + (c[1] - '1');
        ut->gpio_in_val ^= (1u << bit);
        ut->gpio_in_dirty = true;
        cmd_prompt();
        return;
    }

    /* unknown command */
    cmd_prompt();
}

static void cmd_handle_char(ui_term_t *ut, u8 ch)
{
    if (ch == '\r' || ch == '\n') {
        cmd_process(ut);
        return;
    }
    if (ch == 0x7f || ch == '\b') {
        /* backspace */
        if (ut->cmd_len > 0) {
            ut->cmd_len--;
            write(STDOUT_FILENO, "\b \b", 3);
        }
        return;
    }
    if (ut->cmd_len >= CMD_BUF_SIZE - 1) {
        return;
    }
    if (!isprint(ch)) {
        return;
    }
    ut->cmd_buf[ut->cmd_len++] = (char)ch;
    write(STDOUT_FILENO, &ch, 1);
}

static void reset(void *ctx)
{
    ui_term_t *ut = (ui_term_t *)ctx;
    stop_input_thread(ut);

    pthread_mutex_lock(&ut->lock);
    ut->rd = 0;
    ut->wr = 0;
    ut->reset_req = false;
    ut->cmd_mode = false;
    ut->cmd_len = 0;
    ut->gpio_in_dirty = false;
    ut->input_thread_stop = false;
    pthread_mutex_unlock(&ut->lock);

    if (isatty(STDIN_FILENO)) {
        tcgetattr(STDIN_FILENO, &ut->orig_termios);
        struct termios raw = ut->orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        ut->tty_raw = true;
    } else {
        ut->tty_raw = false;
    }

    if (pthread_create(&ut->input_thread, NULL, input_thread_main, ut) == 0) {
        ut->input_thread_valid = true;
    }
}

static void uart_out(void *ctx, u8 ch)
{
    (void)ctx;
    putchar((int)ch);
    fflush(stdout);
}

static bool uart_in(void *ctx, u8 *ch)
{
    ui_term_t *ut = (ui_term_t *)ctx;

    pthread_mutex_lock(&ut->lock);
    if (buf_empty(ut)) {
        pthread_mutex_unlock(&ut->lock);
        return false;
    }
    u8 c = ut->buf[ut->rd];
    ut->rd = (ut->rd + 1u) % BUF_SIZE;
    pthread_cond_signal(&ut->space_cond);
    pthread_mutex_unlock(&ut->lock);

    if (c == 0x1b) {
        /* ESC: enter cmd mode (vim-style) */
        if (!ut->cmd_mode) {
            ut->cmd_mode = true;
            ut->cmd_len = 0;
            cmd_prompt();
        }
        return false;
    }

    if (ut->cmd_mode) {
        cmd_handle_char(ut, c);
        return false;
    }

    *ch = c;
    return true;
}

static bool reset_pending(void *ctx)
{
    ui_term_t *ut = (ui_term_t *)ctx;
    return ut->reset_req;
}

static bool gpio_in_poll(void *ctx, u32 *val)
{
    ui_term_t *ut = (ui_term_t *)ctx;
    if (!ut->gpio_in_dirty) {
        return false;
    }
    *val = ut->gpio_in_val;
    ut->gpio_in_dirty = false;
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
    stop_input_thread(ut);
    if (ut->tty_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &ut->orig_termios);
    }
    pthread_cond_destroy(&ut->space_cond);
    pthread_mutex_destroy(&ut->lock);
    free(ut);
}

sim_ui_t *ui_term_create(void)
{
    ui_term_t *ut = calloc(1, sizeof(ui_term_t));
    pthread_mutex_init(&ut->lock, NULL);
    pthread_cond_init(&ut->space_cond, NULL);
    ut->ui.reset = reset;
    ut->ui.reset_pending = reset_pending;
    ut->ui.uart_out = uart_out;
    ut->ui.uart_in = uart_in;
    ut->ui.gpio_change = gpio_change;
    ut->ui.gpio_in_poll = gpio_in_poll;
    ut->ui.cleanup = cleanup;
    return &ut->ui;
}
