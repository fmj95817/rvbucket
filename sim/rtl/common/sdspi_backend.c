#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static FILE *sd_image;
static uint64_t sd_image_size;
static int sd_image_read_only;

#ifdef __cplusplus
extern "C" {
#endif

int sdspi_backend_open(const char *path)
{
    if (sd_image != NULL) {
        fclose(sd_image);
        sd_image = NULL;
    }
    sd_image_size = 0;
    sd_image_read_only = 0;
    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    sd_image = fopen(path, "r+b");
    if (sd_image == NULL) {
        sd_image = fopen(path, "rb");
        sd_image_read_only = sd_image != NULL;
    }
    if (sd_image == NULL || fseeko(sd_image, 0, SEEK_END) != 0) {
        if (sd_image != NULL) fclose(sd_image);
        sd_image = NULL;
        return 0;
    }
    off_t size = ftello(sd_image);
    if (size < 0 || fseeko(sd_image, 0, SEEK_SET) != 0) {
        fclose(sd_image);
        sd_image = NULL;
        return 0;
    }
    sd_image_size = (uint64_t)size;
    return sd_image_size >= 512;
}

uint64_t sdspi_backend_size(void)
{
    return sd_image_size;
}

int sdspi_backend_read_only(void)
{
    return sd_image_read_only;
}

int sdspi_backend_read_word(uint64_t offset, uint32_t *data)
{
    if (sd_image == NULL || data == NULL || offset > sd_image_size ||
        sizeof(*data) > sd_image_size - offset) {
        return 0;
    }
    return pread(fileno(sd_image), data, sizeof(*data), (off_t)offset) ==
        (ssize_t)sizeof(*data);
}

int sdspi_backend_write_word(uint64_t offset, uint32_t data)
{
    if (sd_image == NULL || sd_image_read_only || offset > sd_image_size ||
        sizeof(data) > sd_image_size - offset) {
        return 0;
    }
    return pwrite(fileno(sd_image), &data, sizeof(data), (off_t)offset) ==
        (ssize_t)sizeof(data);
}

void sdspi_backend_close(void)
{
    if (sd_image != NULL) {
        fclose(sd_image);
        sd_image = NULL;
    }
    sd_image_size = 0;
    sd_image_read_only = 0;
}

#ifdef __cplusplus
}
#endif
