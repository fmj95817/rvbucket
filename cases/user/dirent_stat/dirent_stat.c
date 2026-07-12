#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fail(const char *name)
{
    printf("dirent_stat: FAIL %s errno=%d\n", name, errno);
    return 1;
}

static int touch_file(const char *path, const char *data)
{
    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) {
        return fail("open");
    }

    size_t len = strlen(data);
    if (write(fd, data, len) != (ssize_t)len) {
        close(fd);
        return fail("write");
    }

    if (close(fd) != 0) {
        return fail("close");
    }
    return 0;
}

int main(void)
{
    const char *dir = "/tmp/rvbucket_dirent";
    const char *alpha = "/tmp/rvbucket_dirent/alpha.txt";
    const char *beta = "/tmp/rvbucket_dirent/beta.txt";
    const char *gamma = "/tmp/rvbucket_dirent/gamma.txt";

    if (mkdir("/tmp", 0777) != 0 && errno != EEXIST) {
        return fail("mkdir_tmp");
    }
    if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
        return fail("mkdir_dir");
    }

    if (touch_file(alpha, "alpha") != 0 ||
        touch_file(beta, "beta-data") != 0) {
        return 1;
    }

    DIR *dp = opendir(dir);
    if (dp == NULL) {
        return fail("opendir");
    }

    int found_alpha = 0;
    int found_beta = 0;
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, "alpha.txt") == 0) {
            found_alpha = 1;
        } else if (strcmp(de->d_name, "beta.txt") == 0) {
            found_beta = 1;
        }
    }
    if (closedir(dp) != 0) {
        return fail("closedir");
    }
    if (!found_alpha || !found_beta) {
        return fail("readdir");
    }

    struct stat st;
    if (stat(beta, &st) != 0 || st.st_size != 9) {
        return fail("stat");
    }
    if (rename(beta, gamma) != 0) {
        return fail("rename");
    }
    if (access(gamma, R_OK) != 0) {
        return fail("access");
    }

    if (unlink(alpha) != 0 || unlink(gamma) != 0 || rmdir(dir) != 0) {
        return fail("cleanup");
    }

    puts("dirent_stat: PASS");
    return 0;
}

