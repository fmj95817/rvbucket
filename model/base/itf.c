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

static inline void signal_itf_add_vcd(itf_t *itf)
{
    DBG_VCD_ITF_SCOPE(itf->name);
    dbg_vcd_add_sig("wr_vld", DBG_SIG_TYPE_WIRE, 1, &itf->ctx.signal.write_vld);
    itf->reg_vcd(itf->ctx.signal.shared_pkt_data, DBG_SIG_TYPE_WIRE);
}

static void signal_itf_construct(itf_t *itf, const char *name, const itf_conf_t *conf)
{
    DBG_CHECK(itf->mode == ITF_MODE_SIGNAL);
    DBG_CHECK(name != NULL);
    DBG_CHECK(conf != NULL);

    itf->ctx.signal.ext_sig_src = conf->ext_sig_src;
    if (conf->ext_sig_src) {
        itf->ctx.signal.shared_pkt_data = NULL;
    } else {
        itf->ctx.signal.shared_pkt_data = malloc(conf->pkt_size);
        memset(itf->ctx.signal.shared_pkt_data, 0, conf->pkt_size);
    }

    itf->ctx.signal.old_pkt_data = malloc(conf->pkt_size);
    memset(itf->ctx.signal.old_pkt_data, 0, conf->pkt_size);

    itf->ctx.signal.sig_wr_cb = NULL;
    itf->ctx.signal.sig_wr_cb_args = NULL;
    itf->ctx.signal.write_vld = false;

    if (itf->dump_enable) {
        char dump_path[1024];
        sprintf(dump_path, ITF_DUMP_DIR"/%s_drv.txt", name);
        itf->ctx.signal.dump_fp = fopen(dump_path, "w");
    } else {
        itf->ctx.signal.dump_fp = NULL;
    }

    if (itf->vcd_enable && !conf->ext_sig_src) {
        signal_itf_add_vcd(itf);
    }
}

static inline void *get_fifo_pkt_addr(itf_t *itf, u32 index)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    return (void *)((u8 *)itf->ctx.fifo.pkts_data + index * itf->pkt_size);
}

static void fifo_itf_construct(itf_t *itf, const char *name, const itf_conf_t *conf)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    DBG_CHECK(name != NULL);
    DBG_CHECK(conf != NULL);

    itf->ctx.fifo.fifo_depth = conf->fifo_depth;
    itf->ctx.fifo.pkts_data = malloc(conf->pkt_size * conf->fifo_depth);
    itf->ctx.fifo.pkt_num = 0;
    itf->ctx.fifo.rptr = 0;
    itf->ctx.fifo.wptr = 0;

    if (itf->dump_enable) {
        char slv_path[1024];
        char mst_path[1024];
        sprintf(slv_path, ITF_DUMP_DIR"/%s_slv.txt", name);
        sprintf(mst_path, ITF_DUMP_DIR"/%s_mst.txt", name);
        itf->ctx.fifo.dump_slv_fp = fopen(slv_path, "w");
        itf->ctx.fifo.dump_mst_fp = fopen(mst_path, "w");
    } else {
        itf->ctx.fifo.dump_slv_fp = NULL;
        itf->ctx.fifo.dump_mst_fp = NULL;
    }

    if (itf->vcd_enable) {
        itf->ctx.fifo.pkts_pend_mask = malloc(sizeof(bool) * conf->fifo_depth);
        memset(itf->ctx.fifo.pkts_pend_mask, 0, conf->fifo_depth);

        itf->ctx.fifo.read_vld = false;
        itf->ctx.fifo.write_vld = false;
        itf->ctx.fifo.read_pkt = malloc(conf->pkt_size);
        itf->ctx.fifo.write_pkt = malloc(conf->pkt_size);

        DBG_VCD_ITF_SCOPE(name);
        {
            DBG_VCD_ITF_SCOPE("fifo_pkts");
            char pkt_name[32];
            for (u32 i = 0; i < conf->fifo_depth; i++) {
                sprintf(pkt_name, "pkt [%u]", i);
                DBG_VCD_ITF_SCOPE(pkt_name);
                dbg_vcd_add_sig("pend", DBG_SIG_TYPE_REG, 1, &itf->ctx.fifo.pkts_pend_mask[i]);
                itf->reg_vcd(get_fifo_pkt_addr(itf, i), DBG_SIG_TYPE_REG);
            }
        }
        {
            DBG_VCD_ITF_SCOPE("slv");
            dbg_vcd_add_sig("vld", DBG_SIG_TYPE_WIRE, 1, &itf->ctx.fifo.read_vld);
            itf->reg_vcd(itf->ctx.fifo.read_pkt, DBG_SIG_TYPE_REG);
        }
        {
            DBG_VCD_ITF_SCOPE("mst");
            dbg_vcd_add_sig("vld", DBG_SIG_TYPE_WIRE, 1, &itf->ctx.fifo.write_vld);
            itf->reg_vcd(itf->ctx.fifo.write_pkt, DBG_SIG_TYPE_REG);
        }
    } else {
        itf->ctx.fifo.pkts_pend_mask = NULL;
        itf->ctx.fifo.read_pkt = NULL;
        itf->ctx.fifo.write_pkt = NULL;
    }
}

void itf_construct(itf_t *itf, const char *name, const itf_conf_t *conf)
{
    DBG_CHECK(name != NULL);
    DBG_CHECK(conf != NULL);
    DBG_CHECK(conf->pkt2str != NULL);
    DBG_CHECK(conf->reg_vcd != NULL);

    itf->cycle = conf->cycle;
    itf->name = name;
    itf->mode = conf->mode;
    itf->pkt_size = conf->pkt_size;
    DBG_CHECK(itf->mode <= ITF_MODE_MAX);

    itf->dump_enable = dbg_get_bool_env(ITF_DUMP_ENV) && (!conf->force_disable_dump);
    itf->vcd_enable = dbg_get_bool_env(ITF_VCD_ENV);
    itf->pkt2str = conf->pkt2str;
    itf->reg_vcd = conf->reg_vcd;

    if (itf->dump_enable) {
        struct stat s;
        if (stat(ITF_DUMP_DIR, &s) || !S_ISDIR(s.st_mode)) {
            mkdir(ITF_DUMP_DIR, 0755);
        }
    }

    typedef void (*itf_construct_t)(itf_t *, const char *, const itf_conf_t *);
    itf_construct_t construct_list[ITF_MODE_NUM] = {
        [ITF_MODE_SIGNAL] = &signal_itf_construct,
        [ITF_MODE_FIFO] = &fifo_itf_construct
    };
    itf_construct_t construct = construct_list[itf->mode];
    DBG_CHECK(construct != NULL);
    construct(itf, name, conf);
}

void itf_reset(itf_t *itf)
{
    if (itf->mode == ITF_MODE_SIGNAL) {
        itf->ctx.signal.write_vld = false;
    } else if (itf->mode == ITF_MODE_FIFO) {
        itf->ctx.fifo.pkt_num = 0;
        itf->ctx.fifo.rptr = 0;
        itf->ctx.fifo.wptr = 0;
        if (itf->vcd_enable) {
            memset(itf->ctx.fifo.pkts_pend_mask, 0, sizeof(bool) * itf->ctx.fifo.fifo_depth);
        }
        itf->ctx.fifo.read_vld = false;
        itf->ctx.fifo.write_vld = false;
    }
}

static void signal_itf_free(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_SIGNAL);

    if ((itf->ctx.signal.shared_pkt_data != NULL) &&
        (!itf->ctx.signal.ext_sig_src)) {
        free(itf->ctx.signal.shared_pkt_data);
        itf->ctx.signal.shared_pkt_data = NULL;
    }

    if (itf->ctx.signal.old_pkt_data != NULL) {
        free(itf->ctx.signal.old_pkt_data);
        itf->ctx.signal.old_pkt_data = NULL;
    }

    if (itf->ctx.signal.dump_fp) {
        fclose(itf->ctx.signal.dump_fp);
        itf->ctx.signal.dump_fp = NULL;
    }
}

static void fifo_itf_free(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);

    if (itf->ctx.fifo.pkts_data != NULL) {
        free(itf->ctx.fifo.pkts_data);
        itf->ctx.fifo.pkts_data = NULL;
    }

    if (itf->ctx.fifo.read_pkt != NULL) {
        free(itf->ctx.fifo.read_pkt);
        itf->ctx.fifo.read_pkt = NULL;
    }

    if (itf->ctx.fifo.write_pkt != NULL) {
        free(itf->ctx.fifo.write_pkt);
        itf->ctx.fifo.write_pkt = NULL;
    }

    if (itf->ctx.fifo.pkts_pend_mask != NULL) {
        free(itf->ctx.fifo.pkts_pend_mask);
        itf->ctx.fifo.pkts_pend_mask = NULL;
    }

    if (itf->ctx.fifo.dump_slv_fp) {
        fclose(itf->ctx.fifo.dump_slv_fp);
        itf->ctx.fifo.dump_slv_fp = NULL;
    }

    if (itf->ctx.fifo.dump_mst_fp) {
        fclose(itf->ctx.fifo.dump_mst_fp);
        itf->ctx.fifo.dump_mst_fp = NULL;
    }
}

void itf_free(itf_t *itf)
{
    typedef void (*itf_free_t)(itf_t *);
    itf_free_t free_list[ITF_MODE_NUM] = {
        [ITF_MODE_SIGNAL] = &signal_itf_free,
        [ITF_MODE_FIFO] = &fifo_itf_free
    };
    itf_free_t free_func = free_list[itf->mode];
    DBG_CHECK(free_func != NULL);
    free_func(itf);
}

static void signal_itf_call_wcb(itf_t *itf)
{
    if (itf->ctx.signal.sig_wr_cb != NULL) {
        itf->ctx.signal.sig_wr_cb(itf->ctx.signal.sig_wr_cb_args);
    }

    if (itf->vcd_enable) {
        itf->ctx.signal.write_vld = true;
        itf->ctx.signal.write_cycle = *itf->cycle;
    }
}

static void signal_itf_write(itf_t *itf, const void *pkt)
{
    DBG_CHECK(itf->mode == ITF_MODE_SIGNAL);

    memcpy(itf->ctx.signal.shared_pkt_data, pkt, itf->pkt_size);
    signal_itf_call_wcb(itf);
}

static void fifo_itf_write(itf_t *itf, const void *pkt)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    DBG_CHECK(itf->ctx.fifo.pkt_num < itf->ctx.fifo.fifo_depth);

    if (itf->vcd_enable) {
        memcpy(itf->ctx.fifo.write_pkt, pkt, itf->pkt_size);
        itf->ctx.fifo.pkts_pend_mask[itf->ctx.fifo.wptr] = true;
        itf->ctx.fifo.write_vld = true;
        itf->ctx.fifo.write_cycle = *itf->cycle;
    }

    memcpy(get_fifo_pkt_addr(itf, itf->ctx.fifo.wptr), pkt, itf->pkt_size);
    itf->ctx.fifo.pkt_num++;
    itf->ctx.fifo.wptr = (itf->ctx.fifo.wptr + 1) % itf->ctx.fifo.fifo_depth;

    if (itf->dump_enable) {
        if (itf->ctx.fifo.dump_mst_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->ctx.fifo.dump_mst_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.fifo.dump_mst_fp);
        }
    }
}

void itf_write(itf_t *itf, const void *pkt)
{
    typedef void (*itf_write_t)(itf_t *, const void *);
    itf_write_t write_list[ITF_MODE_NUM] = {
        [ITF_MODE_SIGNAL] = &signal_itf_write,
        [ITF_MODE_FIFO] = &fifo_itf_write
    };
    itf_write_t write_func = write_list[itf->mode];
    DBG_CHECK(write_func != NULL);
    write_func(itf, pkt);
}

static void signal_itf_read(itf_t *itf, void *pkt)
{
    DBG_CHECK(itf->mode == ITF_MODE_SIGNAL);
    memcpy(pkt, itf->ctx.signal.shared_pkt_data, itf->pkt_size);
}

static void fifo_itf_read(itf_t *itf, void *pkt)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    DBG_CHECK(itf->ctx.fifo.pkt_num > 0);

    if (itf->vcd_enable) {
        memcpy(itf->ctx.fifo.read_pkt, get_fifo_pkt_addr(itf, itf->ctx.fifo.rptr), itf->pkt_size);
        itf->ctx.fifo.pkts_pend_mask[itf->ctx.fifo.rptr] = false;
        itf->ctx.fifo.read_vld = true;
        itf->ctx.fifo.read_cycle = *itf->cycle;
    }

    memcpy(pkt, get_fifo_pkt_addr(itf, itf->ctx.fifo.rptr), itf->pkt_size);
    itf->ctx.fifo.pkt_num--;
    itf->ctx.fifo.rptr = (itf->ctx.fifo.rptr + 1) % itf->ctx.fifo.fifo_depth;

    if (itf->dump_enable) {
        if (itf->ctx.fifo.dump_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->ctx.fifo.dump_slv_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.fifo.dump_slv_fp);
        }
    }
}

void itf_read(itf_t *itf, void *pkt)
{
    typedef void (*itf_read_t)(itf_t *, void *);
    itf_read_t read_list[ITF_MODE_NUM] = {
        [ITF_MODE_SIGNAL] = &signal_itf_read,
        [ITF_MODE_FIFO] = &fifo_itf_read
    };
    itf_read_t read_func = read_list[itf->mode];
    DBG_CHECK(read_func != NULL);
    read_func(itf, pkt);
}

static void signal_itf_dbg_clock(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_SIGNAL);

    if (itf->dump_enable && itf->ctx.signal.dump_fp != NULL) {
        void *old = itf->ctx.signal.old_pkt_data;
        void *cur = itf->ctx.signal.shared_pkt_data;
        if (memcmp(cur, old, itf->pkt_size) != 0) {
            char pkt_str[1024];
            itf->pkt2str(cur, pkt_str);
            fprintf(itf->ctx.signal.dump_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.signal.dump_fp);
            memcpy(old, cur, itf->pkt_size);
        }
    }

    if (itf->vcd_enable) {
        u64 delta_cycle = *itf->cycle - itf->ctx.signal.write_cycle;
        if (itf->ctx.signal.write_vld && (delta_cycle == 1u)) {
            itf->ctx.signal.write_vld = false;
        }
    }
}

static void fifo_itf_dbg_clock(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    if (!itf->vcd_enable) {
        return;
    }

    u64 delta_cycle = *itf->cycle - itf->ctx.fifo.read_cycle;
    if (itf->ctx.fifo.read_vld && (delta_cycle == 1u)) {
        itf->ctx.fifo.read_vld = false;
    }

    delta_cycle = *itf->cycle - itf->ctx.fifo.write_cycle;
    if (itf->ctx.fifo.write_vld && (delta_cycle == 1u)) {
        itf->ctx.fifo.write_vld = false;
    }
}

void itf_dbg_clock(itf_t *itf)
{
    typedef void (*itf_dbg_clock_t)(itf_t *);
    itf_dbg_clock_t dbg_clock_list[ITF_MODE_NUM] = {
        [ITF_MODE_SIGNAL] = &signal_itf_dbg_clock,
        [ITF_MODE_FIFO] = &fifo_itf_dbg_clock
    };
    itf_dbg_clock_t dbg_clock_func = dbg_clock_list[itf->mode];
    DBG_CHECK(dbg_clock_func != NULL);
    dbg_clock_func(itf);
}

void *itf_signal_get_src_and_chk(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_SIGNAL);
    DBG_CHECK(itf != NULL);
    DBG_CHECK(itf->ctx.signal.shared_pkt_data != NULL);
    return itf->ctx.signal.shared_pkt_data;
}

void itf_signal_set_src(itf_t *itf, void *src)
{
    DBG_CHECK(src != NULL);
    DBG_CHECK(itf->name != NULL);
    DBG_CHECK(itf->mode == ITF_MODE_SIGNAL);
    DBG_CHECK(itf->ctx.signal.ext_sig_src);
    DBG_CHECK(itf->ctx.signal.shared_pkt_data == NULL);

    itf->ctx.signal.shared_pkt_data = src;
    if (itf->vcd_enable) {
        signal_itf_add_vcd(itf);
    }
}

void itf_signal_write_notify(itf_t *itf)
{
    signal_itf_call_wcb(itf);
}

void itf_signal_set_wcb(itf_t *itf, sig_wr_cb_t cb, void *args)
{
    itf->ctx.signal.sig_wr_cb = cb;
    itf->ctx.signal.sig_wr_cb_args = args;
}

void itf_fifo_get_front(itf_t *itf, void *pkt)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    DBG_CHECK(itf->ctx.fifo.pkt_num > 0);
    memcpy(pkt, get_fifo_pkt_addr(itf, itf->ctx.fifo.rptr), itf->pkt_size);
}

void itf_fifo_pop_front(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    DBG_CHECK(itf->ctx.fifo.pkt_num > 0);

    if (itf->vcd_enable) {
        memcpy(itf->ctx.fifo.read_pkt, get_fifo_pkt_addr(itf, itf->ctx.fifo.rptr), itf->pkt_size);
        itf->ctx.fifo.pkts_pend_mask[itf->ctx.fifo.rptr] = false;
        itf->ctx.fifo.read_vld = true;
        itf->ctx.fifo.read_cycle = *itf->cycle;
    }

    if (itf->dump_enable) {
        if (itf->ctx.fifo.dump_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(get_fifo_pkt_addr(itf, itf->ctx.fifo.rptr), pkt_str);
            fprintf(itf->ctx.fifo.dump_slv_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.fifo.dump_slv_fp);
        }
    }

    itf->ctx.fifo.pkt_num--;
    itf->ctx.fifo.rptr = (itf->ctx.fifo.rptr + 1) % itf->ctx.fifo.fifo_depth;
}

void itf_fifo_pop_all(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);

    while (itf->ctx.fifo.pkt_num > 0) {
        itf_fifo_pop_front(itf);
    }
}

bool itf_fifo_empty(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    return itf->ctx.fifo.pkt_num == 0;
}

bool itf_fifo_full(itf_t *itf)
{
    DBG_CHECK(itf->mode == ITF_MODE_FIFO);
    return itf->ctx.fifo.pkt_num == itf->ctx.fifo.fifo_depth;
}
