#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int fail(const char *name)
{
    printf("syscall_fs_ipc: FAIL %s errno=%d\n", name, errno);
    return 1;
}

static int check_file_io(void)
{
    const char *path = "/tmp/rvbucket_user_fs_ipc.txt";
    const char msg[] = "rvbucket-user-file\n";
    char buf[sizeof(msg)] = {};
    struct stat st;

    if (mkdir("/tmp", 0777) != 0 && errno != EEXIST) {
        return fail("mkdir");
    }

    int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        return fail("open");
    }

    if (write(fd, msg, sizeof(msg) - 1u) != (ssize_t)(sizeof(msg) - 1u)) {
        close(fd);
        return fail("write");
    }
    if (lseek(fd, 0, SEEK_SET) != 0) {
        close(fd);
        return fail("lseek");
    }
    if (read(fd, buf, sizeof(msg) - 1u) != (ssize_t)(sizeof(msg) - 1u)) {
        close(fd);
        return fail("read");
    }
    if (strcmp(buf, msg) != 0) {
        close(fd);
        return fail("strcmp");
    }
    if (fstat(fd, &st) != 0 || st.st_size != (off_t)(sizeof(msg) - 1u)) {
        close(fd);
        return fail("fstat");
    }
    if (close(fd) != 0) {
        return fail("close");
    }
    if (unlink(path) != 0) {
        return fail("unlink");
    }

    return 0;
}

static int check_pipe_fork(void)
{
    int fds[2];
    const char child_msg[] = "pipe-child";
    char buf[sizeof(child_msg)] = {};

    if (pipe(fds) != 0) {
        return fail("pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return fail("fork");
    }

    if (pid == 0) {
        close(fds[0]);
        ssize_t n = write(fds[1], child_msg, sizeof(child_msg) - 1u);
        close(fds[1]);
        _exit(n == (ssize_t)(sizeof(child_msg) - 1u) ? 23 : 24);
    }

    close(fds[1]);
    if (read(fds[0], buf, sizeof(child_msg) - 1u) !=
        (ssize_t)(sizeof(child_msg) - 1u)) {
        close(fds[0]);
        return fail("pipe_read");
    }
    close(fds[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) != pid) {
        return fail("waitpid");
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 23) {
        return fail("child_status");
    }
    if (strcmp(buf, child_msg) != 0) {
        return fail("pipe_payload");
    }

    return 0;
}

int main(void)
{
    if (check_file_io() != 0) {
        return 1;
    }
    if (check_pipe_fork() != 0) {
        return 1;
    }

    puts("syscall_fs_ipc: PASS");
    return 0;
}

