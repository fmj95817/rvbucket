#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int fail(const char *name)
{
    printf("fork_exec: FAIL %s errno=%d\n", name, errno);
    return 1;
}

static int run_child(const char *path, int exp_status)
{
    pid_t pid = fork();
    if (pid < 0) {
        return fail("fork");
    }

    if (pid == 0) {
        execl(path, path, (char *)NULL);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) != pid) {
        return fail("waitpid");
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != exp_status) {
        return fail("child_status");
    }

    return 0;
}

int main(void)
{
    if (run_child("/bin/true", 0) != 0) {
        return 1;
    }
    if (run_child("/bin/false", 1) != 0) {
        return 1;
    }

    puts("fork_exec: PASS");
    return 0;
}

