#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void to_c_array(const void *data, size_t size)
{
    printf("#include \"types.h\"\n\n");
    printf("u32 g_boot_code_size = 0x%lx;\n", size);
    printf("u8 g_boot_code[] = {\n    ");

    uint8_t *bytes = (uint8_t *)data;
    for (int i = 0; i < size; i++) {
        if (i == size - 1) {
            printf("0x%02x\n", bytes[i]);
            break;
        }
        printf("0x%02x, ", bytes[i]);
        if (((i + 1) % 8) == 0) {
            printf("\n    ");
        }
    }

    printf("};\n");
}

static void to_hex(const void *data, size_t size)
{
    uint32_t *word = (uint32_t *)data;
    for (int i = 0; i < size / 4; i++) {
        printf("%08x\n", word[i]);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        return 0;
    }

    const char *bin_path = argv[1];
    FILE *fp = fopen(bin_path, "r");
    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    void *data = malloc(size);
    if (fread(data, 1, size, fp) != size) {
        free(data);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    const char *format = argv[2];
    if (strcmp(format, "c_array") == 0) {
        to_c_array(data, size);
    } else if (strcmp(format, "hex") == 0) {
        to_hex(data, size);
    }

    free(data);
    return 0;
}
