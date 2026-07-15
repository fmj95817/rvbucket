#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "linux_loader.h"
#include "dbg/chk.h"
#include "dbg/log.h"
#include "spec/soc.h"

#define BIN_TYPE_LINUX 1u
#define LINUX_BIN_HEADER_SIZE 40u

typedef struct program_header {
    u32 type;
    u32 itcm_size;
    u32 dtcm_size;
    u32 kernel_size;
    u32 initrd_size;
    u32 dtb_size;
    u32 kernel_load;
    u32 initrd_load;
    u32 dtb_load;
    u32 padding;
} program_header_t;

static inline u32 align4(u32 val)
{
    return (val + 3u) & ~3u;
}

sim_program_t sim_program_read(const char *path)
{
    sim_program_t program = {0};
    DBG_CHECK(path != NULL);

    FILE *fp = fopen(path, "rb");
    DBG_CHECK(fp != NULL);
    DBG_CHECK(fseek(fp, 0L, SEEK_END) == 0);
    long size = ftell(fp);
    DBG_CHECK(size >= 0);
    DBG_CHECK(fseek(fp, 0L, SEEK_SET) == 0);

    program.size = (u32)size;
    program.code = malloc(program.size);
    DBG_CHECK(program.code != NULL);
    DBG_CHECK(fread(program.code, 1, program.size, fp) == program.size);
    DBG_CHECK(fclose(fp) == 0);
    return program;
}

void sim_program_free(sim_program_t *program)
{
    if (program == NULL) {
        return;
    }

    free(program->code);
    program->code = NULL;
    program->size = 0;
}

static const char *sim_find_dtbgen(void)
{
    static const char *candidates[] = {
        "tools/dtbgen.py",
        "../../../tools/dtbgen.py",
        "../../tools/dtbgen.py",
        "../tools/dtbgen.py",
    };

    for (u32 i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (access(candidates[i], R_OK) == 0) {
            return candidates[i];
        }
    }

    return NULL;
}

static sim_program_t sim_gen_linux_dtb(const sim_linux_conf_t *conf,
    u32 initrd_load, u32 initrd_size)
{
    char dtb_path[] = "/tmp/rvbucket-dtb-XXXXXX";
    int fd = mkstemp(dtb_path);
    DBG_CHECK(fd >= 0);
    DBG_CHECK(close(fd) == 0);

    const char *dtbgen = sim_find_dtbgen();
    DBG_CHECK(dtbgen != NULL);

    char cmd[512];
    int n = snprintf(cmd, sizeof(cmd),
        "python3 %s --initrd-load 0x%08x --initrd-size %u --out-dtb %s%s",
        dtbgen, initrd_load, initrd_size, dtb_path,
        conf->ufshc_en ? " --ufs" : "");
    DBG_CHECK(n > 0 && (size_t)n < sizeof(cmd));
    DBG_CHECK(system(cmd) == 0);

    sim_program_t dtb = sim_program_read(dtb_path);
    DBG_CHECK(unlink(dtb_path) == 0);
    DBG_CHECK(dtb.code != NULL);
    return dtb;
}

static bool program_is_linux(const sim_program_t *program)
{
    if (program->size < LINUX_BIN_HEADER_SIZE) {
        return false;
    }

    program_header_t *header = (program_header_t *)program->code;
    return header->type == BIN_TYPE_LINUX;
}

void sim_linux_append_dtb(const sim_linux_conf_t *conf, sim_program_t *program)
{
    DBG_CHECK(conf != NULL);
    DBG_CHECK(program != NULL);

    if (!program_is_linux(program)) {
        return;
    }

    program_header_t *header = (program_header_t *)program->code;
    sim_program_t dtb = sim_gen_linux_dtb(conf, header->initrd_load,
                                          header->initrd_size);

    u32 dtb_off = LINUX_BIN_HEADER_SIZE +
                  align4(header->itcm_size) +
                  align4(header->dtcm_size) +
                  align4(header->kernel_size) +
                  align4(header->initrd_size);
    DBG_CHECK(dtb_off <= program->size);

    u32 dtb_size = dtb.size;
    u32 new_size = dtb_off + align4(dtb_size);
    u8 *new_code = calloc(1, new_size);
    DBG_CHECK(new_code != NULL);
    memcpy(new_code, program->code, dtb_off);
    memcpy(new_code + dtb_off, dtb.code, dtb_size);
    sim_program_free(program);
    sim_program_free(&dtb);

    program->code = new_code;
    program->size = new_size;
    header = (program_header_t *)program->code;
    header->dtb_size = dtb_size;

    DBG_LOG(LOG_PRINT, "sim_top: appended generated linux DTB: %u bytes @ 0x%08x\n",
            header->dtb_size, header->dtb_load);
}

void sim_linux_preload(const sim_linux_conf_t *conf, ram_t *ddr, sim_program_t *program)
{
    DBG_CHECK(conf != NULL);
    DBG_CHECK(ddr != NULL);
    DBG_CHECK(program != NULL);

    if (!program_is_linux(program)) {
        return;
    }

    program_header_t *header = (program_header_t *)program->code;

    u32 itcm_size = header->itcm_size;
    u32 dtcm_size = header->dtcm_size;
    u32 kernel_size = header->kernel_size;
    u32 initrd_size = header->initrd_size;
    u32 dtb_size = header->dtb_size;
    u32 kernel_load = header->kernel_load;
    u32 initrd_load = header->initrd_load;
    u32 dtb_load = header->dtb_load;

    u32 off = LINUX_BIN_HEADER_SIZE + align4(itcm_size) + align4(dtcm_size);
    DBG_CHECK(off <= program->size);
    DBG_CHECK(off + align4(kernel_size) <= program->size);
    ram_load(ddr, program->code + off, kernel_load - DDR_BASE, kernel_size);

    off += align4(kernel_size);
    DBG_CHECK(off + align4(initrd_size) <= program->size);
    ram_load(ddr, program->code + off, initrd_load - DDR_BASE, initrd_size);

    off += align4(initrd_size);
    DBG_CHECK(off + align4(dtb_size) <= program->size);
    sim_program_t dtb = sim_gen_linux_dtb(conf, initrd_load, initrd_size);
    ram_load(ddr, dtb.code, dtb_load - DDR_BASE, dtb.size);
    u32 generated_dtb_size = dtb.size;
    sim_program_free(&dtb);

    header->kernel_size = 0;
    header->initrd_size = 0;
    header->dtb_size = 0;

    DBG_LOG(LOG_PRINT, "sim_top: preloaded linux payloads to DDR: kernel %u bytes @ 0x%08x,"
        " initrd %u bytes @ 0x%08x, dtb %u bytes @ 0x%08x\n",
        kernel_size, kernel_load, initrd_size, initrd_load, generated_dtb_size, dtb_load);
}
