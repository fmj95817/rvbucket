#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "boot_image.h"

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
    if (argc < 3) {
        return 0;
    }

    bool linux_image = false;
    const char *firmware_path;
    const char *kernel_path = NULL;
    const char *initrd_path = NULL;
    const char *dtb_path = NULL;
    const char *bin_path;
    uint32_t kernel_load = 0;
    uint32_t initrd_load = 0;
    uint32_t dtb_load = 0;

    if (argc >= 10 && strcmp(argv[1], "--linux") == 0) {
        linux_image = true;
        firmware_path = argv[2];
        kernel_path = argv[3];
        initrd_path = argv[4];
        dtb_path = argv[5];
        bin_path = argv[6];
        kernel_load = parse_u32(argv[7]);
        initrd_load = parse_u32(argv[8]);
        dtb_load = parse_u32(argv[9]);
    } else {
        firmware_path = argv[1];
        bin_path = argv[2];
    }

    uint32_t firmware_size = 0;
    uint32_t kernel_size = 0;
    uint32_t initrd_size = 0;
    uint32_t dtb_size = 0;

    void *firmware = read_bin(firmware_path, &firmware_size);
    void *kernel = NULL;
    void *initrd = NULL;
    void *dtb = NULL;
    assert(firmware != NULL);
    if (linux_image) {
        kernel = read_bin(kernel_path, &kernel_size);
        initrd = read_bin(initrd_path, &initrd_size);
        dtb = read_bin(dtb_path, &dtb_size);
        assert(kernel != NULL);
        assert(initrd != NULL);
        assert(dtb != NULL);
    }

    rvb_bin_header_t header = {
        .type = linux_image ? RVB_BIN_TYPE_LINUX : RVB_BIN_TYPE_BARE,
        .firmware_size = firmware_size
    };
    if (linux_image) {
        header.kernel_size = kernel_size;
        header.initrd_size = initrd_size;
        header.dtb_size = dtb_size;
        header.kernel_load = kernel_load;
        header.initrd_load = initrd_load;
        header.dtb_load = dtb_load;
    }

    FILE *bin = fopen(bin_path, "wb");
    fwrite(&header, sizeof(header), 1, bin);
    write_blob_aligned(bin, firmware, firmware_size);
    free(firmware);
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
