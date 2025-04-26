#include "itf.h"
#include <string.h>
#include <sys/stat.h>
#include "dbg/chk.h"
#include "dbg/env.h"

#define ITF_DUMP_DIR "itf_dump"
#define ITF_DUMP_ENV "ITF_DUMP"

void itf_construct(itf_t *itf, const u64 *cycle, const char *name, pkt2str_t pkt2str, u32 pkt_size, u32 fifo_depth)
{
    itf->pkt_size = pkt_size;
    itf->fifo_depth = fifo_depth;

    itf->cycle = cycle;

    itf->dump_enable = dbg_get_bool_env(ITF_DUMP_ENV);
    itf->name = name;
    itf->pkt2str = pkt2str;

    if (itf->dump_enable) {
        struct stat s;
        if (stat(ITF_DUMP_DIR, &s) || !S_ISDIR(s.st_mode)) {
            mkdir(ITF_DUMP_DIR, 0755);
        }
    }

    if (itf->dump_enable) {
        char slv_path[1024];
        char mst_path[1024];
        sprintf(slv_path, ITF_DUMP_DIR"/%s_slv.txt", name);
        sprintf(mst_path, ITF_DUMP_DIR"/%s_mst.txt", name);
        itf->dump_slv_fp = fopen(slv_path, "w");
        itf->dump_mst_fp = fopen(mst_path, "w");
    } else {
        itf->dump_slv_fp = NULL;
        itf->dump_mst_fp = NULL;
    }

    itf->pkts_data = malloc(pkt_size *fifo_depth);
    itf->pkt_num = 0;
    itf->rptr = 0;
    itf->wptr = 0;
}

void itf_free(itf_t *itf)
{
    if (itf->pkts_data != NULL) {
        free(itf->pkts_data);
        itf->pkts_data = NULL;
    }

    if (itf->dump_slv_fp) {
        fclose(itf->dump_slv_fp);
        itf->dump_slv_fp = NULL;
    }

    if (itf->dump_mst_fp) {
        fclose(itf->dump_mst_fp);
        itf->dump_mst_fp = NULL;
    }
}

static inline void *get_pkt_addr(itf_t *itf, u32 index)
{
    return (void *)((u8 *)itf->pkts_data + index * itf->pkt_size);
}

void itf_write(itf_t *itf, const void *pkt)
{
    DBG_CHECK(itf->pkt_num < itf->fifo_depth);

    memcpy(get_pkt_addr(itf, itf->wptr), pkt, itf->pkt_size);
    itf->pkt_num++;
    itf->wptr = (itf->wptr + 1) % itf->fifo_depth;

    if (itf->dump_enable) {
        if (itf->dump_mst_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->dump_mst_fp, U64_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->dump_mst_fp);
        }
    }
}

void itf_read(itf_t *itf, void *pkt)
{
    DBG_CHECK(itf->pkt_num > 0);

    memcpy(pkt, get_pkt_addr(itf, itf->rptr), itf->pkt_size);
    itf->pkt_num--;
    itf->rptr = (itf->rptr + 1) % itf->fifo_depth;

    if (itf->dump_enable) {
        if (itf->dump_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->dump_slv_fp, U64_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->dump_slv_fp);
        }
    }
}

void itf_fifo_get_front(itf_t *itf, void *pkt)
{
    DBG_CHECK(itf->pkt_num > 0);
    memcpy(pkt, get_pkt_addr(itf, itf->rptr), itf->pkt_size);
}

void itf_fifo_pop_front(itf_t *itf)
{
    DBG_CHECK(itf->pkt_num > 0);

    if (itf->dump_enable) {
        if (itf->dump_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(get_pkt_addr(itf, itf->rptr), pkt_str);
            fprintf(itf->dump_slv_fp, U64_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->dump_slv_fp);
        }
    }

    itf->pkt_num--;
    itf->rptr = (itf->rptr + 1) % itf->fifo_depth;
}

bool itf_fifo_empty(itf_t *itf)
{
    return itf->pkt_num == 0;
}

bool itf_fifo_full(itf_t *itf)
{
    return itf->pkt_num == itf->fifo_depth;
}