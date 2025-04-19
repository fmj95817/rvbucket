#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

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

int main(int argc, char *argv[])
{
    if (argc < 4) {
        return 0;
    }

    const char *itcm_path = argv[1];
    const char *dtcm_path = argv[2];
    const char *bin_path = argv[3];

    uint32_t itcm_size = 0;
    uint32_t dtcm_size = 0;

    void *itcm = read_bin(itcm_path, &itcm_size);
    void *dtcm = read_bin(dtcm_path, &dtcm_size);
    assert(itcm != NULL);

    FILE *bin = fopen(bin_path, "w");
    fwrite(&itcm_size, sizeof(itcm_size), 1, bin);
    fwrite(&dtcm_size, sizeof(dtcm_size), 1, bin);

    fwrite(itcm, 1, itcm_size, bin);
    free(itcm);

    if (dtcm != NULL) {
        fwrite(dtcm, 1, dtcm_size, bin);
        free(dtcm);
    }

    fclose(bin);
    return 0;
}