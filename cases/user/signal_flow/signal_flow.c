#include <signal.h>
#include <stdio.h>
#include <string.h>

static volatile sig_atomic_t seen_usr1;
static volatile sig_atomic_t seen_usr2;

static int fail(const char *name)
{
    printf("signal_flow: FAIL %s\n", name);
    return 1;
}

static void handler(int sig, siginfo_t *info, void *ctx)
{
    (void)ctx;

    if (info == NULL || info->si_signo != sig) {
        return;
    }

    if (sig == SIGUSR1) {
        seen_usr1++;
    } else if (sig == SIGUSR2) {
        seen_usr2++;
    }
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        return fail("sigaction_usr1");
    }
    if (sigaction(SIGUSR2, &sa, NULL) != 0) {
        return fail("sigaction_usr2");
    }

    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &block, NULL) != 0) {
        return fail("sigprocmask_block");
    }

    if (raise(SIGUSR1) != 0) {
        return fail("raise_blocked");
    }
    if (seen_usr1 != 0) {
        return fail("blocked_delivery");
    }

    if (sigprocmask(SIG_UNBLOCK, &block, NULL) != 0) {
        return fail("sigprocmask_unblock");
    }
    if (seen_usr1 != 1) {
        return fail("pending_delivery");
    }

    if (raise(SIGUSR2) != 0 || seen_usr2 != 1) {
        return fail("raise_usr2");
    }

    puts("signal_flow: PASS");
    return 0;
}

