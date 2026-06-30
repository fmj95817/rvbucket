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
#ifdef __linux__
#include <sys/prctl.h>
#endif

typedef struct {
    sim_ui_t ui;
    int fd;
    pid_t pid;
    bool reset_req;
    char rbuf[256];   // persistent read buffer for uart_in
    int rpos;         // read position in rbuf
    int rlen;         // valid data length in rbuf
} ui_web_t;

static void reset(void *ctx)
{
    ui_web_t *uw = (ui_web_t *)ctx;
    uw->rpos = 0;
    uw->rlen = 0;
    uw->reset_req = false;
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
    if (strcmp(line, "reset") == 0) {
        uw->reset_req = true;
        return uart_in(ctx, ch);
    }
    if (strncmp(line, "gpin:", 5) == 0) {
        /* GPIO input from Web UI — handled by sim_top polling */
        return uart_in(ctx, ch);
    }
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

static bool reset_pending(void *ctx)
{
    ui_web_t *uw = (ui_web_t *)ctx;
    return uw->reset_req;
}

static bool gpio_in_poll(void *ctx, u32 *val)
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

    /* scan buffer for gpin: line */
    int saved_rpos = uw->rpos;
    for (int i = uw->rpos; i < uw->rlen; i++) {
        if (uw->rbuf[i] != '\n') continue;
        uw->rbuf[i] = '\0';
        char *line = uw->rbuf + saved_rpos;
        uw->rpos = i + 1;

        if (strncmp(line, "gpin:", 5) == 0) {
            unsigned v;
            if (sscanf(line + 5, "%x", &v) == 1) {
                *val = (u32)v;
                return true;
            }
        }
        /* not gpin: — put it back for uart_in to consume */
        uw->rbuf[i] = '\n';
        uw->rpos = saved_rpos;
        break;
    }
    return false;
}

static void gpio_change(void *ctx, u32 val)
{
    ui_web_t *uw = (ui_web_t *)ctx;
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "gpio:%06x\n", val & 0xffffff);
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
#ifdef __linux__
        prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif
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
    uw->ui.reset_pending = reset_pending;
    uw->ui.uart_out = uart_out;
    uw->ui.uart_in = uart_in;
    uw->ui.gpio_change = gpio_change;
    uw->ui.gpio_in_poll = gpio_in_poll;
    uw->ui.cleanup = cleanup;
    return &uw->ui;
}
