#ifndef SIM_MODEL_LINUX_LOADER_H
#define SIM_MODEL_LINUX_LOADER_H

#include "base/types.h"
#include "mem/ram.h"

typedef struct sim_program {
    u32 size;
    u8 *code;
} sim_program_t;

typedef struct sim_linux_conf {
    bool ufshc_en;
} sim_linux_conf_t;

sim_program_t sim_program_read(const char *path);
void sim_program_free(sim_program_t *program);
void sim_linux_append_dtb(const sim_linux_conf_t *conf, sim_program_t *program);
void sim_linux_preload(const sim_linux_conf_t *conf, ram_t *ddr, sim_program_t *program);

#endif
