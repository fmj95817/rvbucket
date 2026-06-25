#include "ui_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>

typedef struct {
    sim_ui_t ui;
    int fd;
    pid_t pid;
    char rbuf[256];   // persistent read buffer for uart_in
    int rpos;         // read position in rbuf
    int rlen;         // valid data length in rbuf
} ui_web_t;

static void reset(void *ctx)
{
    ui_web_t *uw = (ui_web_t *)ctx;
    uw->rpos = 0;
    uw->rlen = 0;
}

static void uart_out(void *ctx, u8 ch)
{
    ui_web_t *uw = (ui_web_t *)ctx;
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "uart:%02x\n", ch);
    write(uw->fd, buf, n);
}

static bool uart_in(void *ctx, u8 *ch)
{
    ui_web_t *uw = (ui_web_t *)ctx;

    /* refill buffer if consumed or empty */
    if (uw->rpos >= uw->rlen) {
        int n = read(uw->fd, uw->rbuf, sizeof(uw->rbuf) - 1);
        if (n <= 0) {
            uw->rpos = uw->rlen = 0;
            return false;
        }
        uw->rbuf[n] = '\0';
        uw->rpos = 0;
        uw->rlen = n;
    }

    /* find the next \n-terminated line starting at rpos */
    int start = uw->rpos;
    int end = start;
    while (end < uw->rlen && uw->rbuf[end] != '\n') {
        end++;
    }
    if (end >= uw->rlen) {
        /* incomplete line — discard */
        uw->rpos = uw->rlen = 0;
        return false;
    }
    uw->rbuf[end] = '\0';  // terminate the line
    uw->rpos = end + 1;    // advance past the \n

    char *line = uw->rbuf + start;
    if (strncmp(line, "in:", 3) == 0) {
        int val;
        if (sscanf(line + 3, "%x", &val) == 1) {
            *ch = (u8)val;
            return true;
        }
    }
    /* line didn't match — try the next one */
    return uart_in(ctx, ch);
}

static void gpio_change(void *ctx, u32 val)
{
    ui_web_t *uw = (ui_web_t *)ctx;
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "gpio:%04x\n", val & 0xffff);
    write(uw->fd, buf, n);
}

static void cleanup(void *ctx)
{
    ui_web_t *uw = (ui_web_t *)ctx;
    if (uw->pid > 0) {
        kill(uw->pid, SIGTERM);
        waitpid(uw->pid, NULL, 0);
    }
    if (uw->fd >= 0) {
        close(uw->fd);
    }
    free(uw);
}

sim_ui_t *ui_web_create(void)
{
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(sv[0]);
        dup2(sv[1], STDIN_FILENO);
        dup2(sv[1], STDOUT_FILENO);
        close(sv[1]);

        char cwd[512];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            exit(1);
        }
        char path[1024];
        snprintf(path, sizeof(path), "%s/../../../tools/web_ui_v2.py", cwd);
        execlp("python3", "python3", path, NULL);
        exit(1);
    }
    close(sv[1]);

    int fl = fcntl(sv[0], F_GETFL, 0);
    fcntl(sv[0], F_SETFL, fl | O_NONBLOCK);

    ui_web_t *uw = calloc(1, sizeof(ui_web_t));
    uw->fd = sv[0];
    uw->pid = pid;
    uw->ui.reset = reset;
    uw->ui.uart_out = uart_out;
    uw->ui.uart_in = uart_in;
    uw->ui.gpio_change = gpio_change;
    uw->ui.cleanup = cleanup;
    return &uw->ui;
}
