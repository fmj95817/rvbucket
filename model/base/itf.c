#include "itf.h"
#include "dbg.h"
#include <string.h>
#include <linux/limits.h>

void itf_construct(itf_t *itf, u64 *cycles, const char *name, pkt2str_t pkt2str, u32 pkt_size, u32 fifo_depth)
{
    itf->pkt_size = pkt_size;
    itf->fifo_depth = fifo_depth;

    itf->cycles = cycles;
    itf->name = name;
    itf->pkt2str = pkt2str;

    if (dbg_get_bool_env("ITF_DUMP")) {
        char slv_path[PATH_MAX];
        char mst_path[PATH_MAX];
        sprintf(slv_path, "itf_dump/%s_slv.txt", name);
        sprintf(mst_path, "itf_dump/%s_mst.txt", name);
        itf->itf_dump_slv_fp = fopen(slv_path, "w");
        itf->itf_dump_mst_fp = fopen(mst_path, "w");
    } else {
        itf->itf_dump_slv_fp = NULL;
        itf->itf_dump_mst_fp = NULL;
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

    if (itf->itf_dump_slv_fp) {
        fclose(itf->itf_dump_slv_fp);
        itf->itf_dump_slv_fp = NULL;
    }

    if (itf->itf_dump_mst_fp) {
        fclose(itf->itf_dump_mst_fp);
        itf->itf_dump_mst_fp = NULL;
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

    if (dbg_get_bool_env("ITF_DUMP")) {
        if (itf->itf_dump_mst_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->itf_dump_mst_fp, "%lu %s", *itf->cycles, pkt_str);
        }
    }
}

void itf_read(itf_t *itf, void *pkt)
{
    DBG_CHECK(itf->pkt_num > 0);

    memcpy(pkt, get_pkt_addr(itf, itf->rptr), itf->pkt_size);
    itf->pkt_num--;
    itf->rptr = (itf->rptr + 1) % itf->fifo_depth;

    if (dbg_get_bool_env("ITF_DUMP")) {
        if (itf->itf_dump_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->itf_dump_slv_fp, "%lu %s", *itf->cycles, pkt_str);
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