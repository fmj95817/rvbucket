#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define BIN_TYPE_BARE  0u
#define BIN_TYPE_LINUX 1u

static void *read_bin(const char *bin_path, uint32_t *psize)
{
    FILE *fp = fopen(bin_path, "r");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    if (size == 0) {
        *psize = 0;
        fclose(fp);
        return NULL;
    }

    void *data = malloc(size);
    if (fread(data, 1, size, fp) != size) {
        free(data);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    *psize = (uint32_t)size;
    return data;
}

static void write_u32(FILE *fp, uint32_t val)
{
    fwrite(&val, sizeof(val), 1, fp);
}

static void write_blob_aligned(FILE *fp, const void *data, uint32_t size)
{
    if (size == 0) {
        return;
    }

    fwrite(data, 1, size, fp);
    uint32_t pad = (-size) & 3u;
    if (pad != 0) {
        uint32_t zero = 0;
        fwrite(&zero, 1, pad, fp);
    }
}

static uint32_t parse_u32(const char *str)
{
    return (uint32_t)strtoul(str, NULL, 0);
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        return 0;
    }

    bool linux_image = false;
    const char *itcm_path;
    const char *dtcm_path;
    const char *kernel_path = NULL;
    const char *initrd_path = NULL;
    const char *dtb_path = NULL;
    const char *bin_path;
    uint32_t kernel_load = 0;
    uint32_t initrd_load = 0;
    uint32_t dtb_load = 0;

    if (argc >= 11 && strcmp(argv[1], "--linux") == 0) {
        linux_image = true;
        itcm_path = argv[2];
        dtcm_path = argv[3];
        kernel_path = argv[4];
        initrd_path = argv[5];
        dtb_path = argv[6];
        bin_path = argv[7];
        kernel_load = parse_u32(argv[8]);
        initrd_load = parse_u32(argv[9]);
        dtb_load = parse_u32(argv[10]);
    } else {
        itcm_path = argv[1];
        dtcm_path = argv[2];
        bin_path = argv[3];
    }

    uint32_t itcm_size = 0;
    uint32_t dtcm_size = 0;
    uint32_t kernel_size = 0;
    uint32_t initrd_size = 0;
    uint32_t dtb_size = 0;

    void *itcm = read_bin(itcm_path, &itcm_size);
    void *dtcm = read_bin(dtcm_path, &dtcm_size);
    void *kernel = NULL;
    void *initrd = NULL;
    void *dtb = NULL;
    assert(itcm != NULL);
    if (linux_image) {
        kernel = read_bin(kernel_path, &kernel_size);
        initrd = read_bin(initrd_path, &initrd_size);
        dtb = read_bin(dtb_path, &dtb_size);
        assert(kernel != NULL);
        assert(initrd != NULL);
        assert(dtb != NULL);
    }

    FILE *bin = fopen(bin_path, "wb");
    write_u32(bin, linux_image ? BIN_TYPE_LINUX : BIN_TYPE_BARE);
    write_u32(bin, itcm_size);
    write_u32(bin, dtcm_size);
    if (linux_image) {
        write_u32(bin, kernel_size);
        write_u32(bin, initrd_size);
        write_u32(bin, dtb_size);
        write_u32(bin, kernel_load);
        write_u32(bin, initrd_load);
        write_u32(bin, dtb_load);
        write_u32(bin, 0);
    } else {
        write_u32(bin, 0);
        write_u32(bin, 0);
        write_u32(bin, 0);
        write_u32(bin, 0);
        write_u32(bin, 0);
        write_u32(bin, 0);
        write_u32(bin, 0);
    }

    write_blob_aligned(bin, itcm, itcm_size);
    free(itcm);

    if (dtcm != NULL) {
        write_blob_aligned(bin, dtcm, dtcm_size);
        free(dtcm);
    }
    if (kernel != NULL) {
        write_blob_aligned(bin, kernel, kernel_size);
        free(kernel);
    }
    if (initrd != NULL) {
        write_blob_aligned(bin, initrd, initrd_size);
        free(initrd);
    }
    if (dtb != NULL) {
        write_blob_aligned(bin, dtb, dtb_size);
        free(dtb);
    }

    fclose(bin);
    return 0;
}
