#include "ui_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>

#define BUF_SIZE 1024

typedef struct {
    sim_ui_t ui;
    struct termios orig_termios;
    bool tty_raw;
    bool input_thread_valid;
    bool input_thread_stop;
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

static void reset(void *ctx)
{
    ui_term_t *ut = (ui_term_t *)ctx;
    stop_input_thread(ut);

    pthread_mutex_lock(&ut->lock);
    ut->rd = 0;
    ut->wr = 0;
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
    *ch = ut->buf[ut->rd];
    ut->rd = (ut->rd + 1u) % BUF_SIZE;
    pthread_cond_signal(&ut->space_cond);
    pthread_mutex_unlock(&ut->lock);
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
    ut->ui.uart_out = uart_out;
    ut->ui.uart_in = uart_in;
    ut->ui.gpio_change = gpio_change;
    ut->ui.cleanup = cleanup;

    return &ut->ui;
}
