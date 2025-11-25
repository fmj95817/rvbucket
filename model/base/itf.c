#include "itf.h"
#include <string.h>
#include <sys/stat.h>
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/env.h"
#include "dbg/vcd.h"

#define ITF_DUMP_DIR "itf_dump"
#define ITF_DUMP_ENV "ITF_DUMP"
#define ITF_VCD_ENV "ITF_VCD"

static inline void *get_pkt_addr(itf_t *itf, u32 index)
{
    return (void *)((u8 *)itf->pkts_data + index * itf->pkt_size);
}

void itf_construct(itf_t *itf, const char *name, const itf_conf_t *conf)
{
    DBG_CHECK(name != NULL);
    DBG_CHECK(conf != NULL);
    DBG_CHECK(conf->pkt2str != NULL);
    DBG_CHECK(conf->reg_vcd != NULL);

    itf->cycle = conf->cycle;

    itf->pkt_size = conf->pkt_size;
    itf->fifo_depth = conf->fifo_depth;
    itf->pkts_data = malloc(conf->pkt_size * conf->fifo_depth);
    itf->pkt_num = 0;
    itf->rptr = 0;
    itf->wptr = 0;

    itf->dump_enable = dbg_get_bool_env(ITF_DUMP_ENV);
    itf->vcd_enable = dbg_get_bool_env(ITF_VCD_ENV);
    itf->name = name;
    itf->pkt2str = conf->pkt2str;
    itf->reg_vcd = conf->reg_vcd;

    if (itf->dump_enable) {
        struct stat s;
        if (stat(ITF_DUMP_DIR, &s) || !S_ISDIR(s.st_mode)) {
            mkdir(ITF_DUMP_DIR, 0755);
        }
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

    if (itf->vcd_enable) {
        itf->pkts_pend_mask = malloc(sizeof(bool) * conf->fifo_depth);
        memset(itf->pkts_pend_mask, 0, conf->fifo_depth);

        itf->read_vld = false;
        itf->write_vld = false;
        itf->read_pkt = malloc(conf->pkt_size);
        itf->write_pkt = malloc(conf->pkt_size);

        DBG_VCD_ITF_SCOPE(name);
        {
            DBG_VCD_ITF_SCOPE("fifo_pkts");
            char pkt_name[32];
            for (u32 i = 0; i < conf->fifo_depth; i++) {
                sprintf(pkt_name, "pkt [%u]", i);
                DBG_VCD_ITF_SCOPE(pkt_name);
                dbg_vcd_add_sig("pend", DBG_SIG_TYPE_REG, 1, &itf->pkts_pend_mask[i]);
                itf->reg_vcd(get_pkt_addr(itf, i));
            }
        }
        {
            DBG_VCD_ITF_SCOPE("slv");
            dbg_vcd_add_sig("vld", DBG_SIG_TYPE_WIRE, 1, &itf->read_vld);
            itf->reg_vcd(itf->read_pkt);
        }
        {
            DBG_VCD_ITF_SCOPE("mst");
            dbg_vcd_add_sig("vld", DBG_SIG_TYPE_WIRE, 1, &itf->write_vld);
            itf->reg_vcd(itf->write_pkt);
        }
    } else {
        itf->pkts_pend_mask = NULL;
        itf->read_pkt = NULL;
        itf->write_pkt = NULL;
    }
}

void itf_free(itf_t *itf)
{
    if (itf->pkts_data != NULL) {
        free(itf->pkts_data);
        itf->pkts_data = NULL;
    }

    if (itf->read_pkt != NULL) {
        free(itf->read_pkt);
        itf->read_pkt = NULL;
    }

    if (itf->write_pkt != NULL) {
        free(itf->write_pkt);
        itf->write_pkt = NULL;
    }

    if (itf->pkts_pend_mask != NULL) {
        free(itf->pkts_pend_mask);
        itf->pkts_pend_mask = NULL;
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

void itf_write(itf_t *itf, const void *pkt)
{
    DBG_CHECK(itf->pkt_num < itf->fifo_depth);

    if (itf->vcd_enable) {
        memcpy(itf->write_pkt, pkt, itf->pkt_size);
        itf->pkts_pend_mask[itf->wptr] = true;
        itf->write_vld = true;
        itf->write_cycle = *itf->cycle;
    }

    memcpy(get_pkt_addr(itf, itf->wptr), pkt, itf->pkt_size);
    itf->pkt_num++;
    itf->wptr = (itf->wptr + 1) % itf->fifo_depth;

    if (itf->dump_enable) {
        if (itf->dump_mst_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->dump_mst_fp, U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->dump_mst_fp);
        }
    }
}

void itf_read(itf_t *itf, void *pkt)
{
    DBG_CHECK(itf->pkt_num > 0);

    if (itf->vcd_enable) {
        memcpy(itf->read_pkt, get_pkt_addr(itf, itf->rptr), itf->pkt_size);
        itf->pkts_pend_mask[itf->rptr] = false;
        itf->read_vld = true;
        itf->read_cycle = *itf->cycle;
    }

    memcpy(pkt, get_pkt_addr(itf, itf->rptr), itf->pkt_size);
    itf->pkt_num--;
    itf->rptr = (itf->rptr + 1) % itf->fifo_depth;

    if (itf->dump_enable) {
        if (itf->dump_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->dump_slv_fp, U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
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

    if (itf->vcd_enable) {
        memcpy(itf->read_pkt, get_pkt_addr(itf, itf->rptr), itf->pkt_size);
        itf->pkts_pend_mask[itf->rptr] = false;
        itf->read_vld = true;
        itf->read_cycle = *itf->cycle;
    }

    if (itf->dump_enable) {
        if (itf->dump_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(get_pkt_addr(itf, itf->rptr), pkt_str);
            fprintf(itf->dump_slv_fp, U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
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

void itf_dbg_clock(itf_t *itf)
{
    if (!itf->vcd_enable) {
        return;
    }

    if (itf->read_vld && ((*itf->cycle - itf->read_cycle) == 1u)) {
        itf->read_vld = false;
    }

    if (itf->write_vld && ((*itf->cycle - itf->write_cycle) == 1u)) {
        itf->write_vld = false;
    }
}
