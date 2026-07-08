#include "itf.h"
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/env.h"
#include "dbg/vcd.h"

#define ITF_TRACE_DIR "itf_trace"
#define ITF_TRACE_ENV "ITF_TRACE"
#define ITF_TRACE_LIST "itf_trace_list.txt"
#define ITF_VCD_ENV "ITF_VCD"
#define ITF_VCD_LIST "itf_vcd_list.txt"

typedef struct itf_name_list {
    u32 name_num;
    char **names;
} itf_name_list_t;

static itf_name_list_t g_itf_trace_list;
static itf_name_list_t g_itf_vcd_list;

static void itf_name_list_load(itf_name_list_t *list, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL) {
        size_t len = strlen(line);
        DBG_CHECK(len == 0 || line[len - 1] == '\n' || feof(fp));

        char *start = line;
        while (*start != '\0' && isspace((unsigned char)*start)) {
            start++;
        }
        char *end = start + strlen(start);
        while (end > start && isspace((unsigned char)end[-1])) {
            end--;
        }
        *end = '\0';
        if (*start == '\0') {
            continue;
        }

        char **names = realloc(list->names, sizeof(*names) * (list->name_num + 1u));
        DBG_CHECK(names != NULL);
        list->names = names;

        size_t name_size = strlen(start) + 1u;
        char *trace_name = malloc(name_size);
        DBG_CHECK(trace_name != NULL);
        memcpy(trace_name, start, name_size);
        list->names[list->name_num++] = trace_name;
    }
    fclose(fp);
}

static bool itf_name_list_match(const itf_name_list_t *list,
    const char *hier_name, const char *name)
{
    char full_name[1024];
    int ret = snprintf(full_name, sizeof(full_name), "%s.%s", hier_name, name);
    DBG_CHECK(ret >= 0 && (u32)ret < sizeof(full_name));
    for (u32 i = 0; i < list->name_num; i++) {
        if (strcmp(list->names[i], full_name) == 0) {
            return true;
        }
    }
    return false;
}

static void itf_name_list_free(itf_name_list_t *list)
{
    for (u32 i = 0; i < list->name_num; i++) {
        free(list->names[i]);
    }
    free(list->names);
}

__attribute__((constructor)) static void itf_name_lists_init(void)
{
    itf_name_list_load(&g_itf_trace_list, ITF_TRACE_LIST);
    itf_name_list_load(&g_itf_vcd_list, ITF_VCD_LIST);
}

__attribute__((destructor)) static void itf_name_lists_free(void)
{
    itf_name_list_free(&g_itf_trace_list);
    itf_name_list_free(&g_itf_vcd_list);
}

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

    if (itf->trace_enable) {
        char trace_path[1024];
        int ret = snprintf(trace_path, sizeof(trace_path), ITF_TRACE_DIR"/%s.%s_drv.txt",
            itf->hier_name, name);
        DBG_CHECK(ret >= 0 && (u32)ret < sizeof(trace_path));
        itf->ctx.signal.trace_fp = fopen(trace_path, "w");
    } else {
        itf->ctx.signal.trace_fp = NULL;
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

    if (itf->trace_enable) {
        char slv_path[1024];
        char mst_path[1024];
        int slv_ret = snprintf(slv_path, sizeof(slv_path), ITF_TRACE_DIR"/%s.%s_slv.txt",
            itf->hier_name, name);
        int mst_ret = snprintf(mst_path, sizeof(mst_path), ITF_TRACE_DIR"/%s.%s_mst.txt",
            itf->hier_name, name);
        DBG_CHECK(slv_ret >= 0 && (u32)slv_ret < sizeof(slv_path));
        DBG_CHECK(mst_ret >= 0 && (u32)mst_ret < sizeof(mst_path));
        itf->ctx.fifo.trace_slv_fp = fopen(slv_path, "w");
        itf->ctx.fifo.trace_mst_fp = fopen(mst_path, "w");
    } else {
        itf->ctx.fifo.trace_slv_fp = NULL;
        itf->ctx.fifo.trace_mst_fp = NULL;
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
    DBG_CHECK(conf->hier_name != NULL);
    DBG_CHECK(conf->pkt2str != NULL);
    DBG_CHECK(conf->reg_vcd != NULL);

    itf->cycle = conf->cycle;
    itf->hier_name = conf->hier_name;
    itf->name = name;
    itf->mode = conf->mode;
    itf->pkt_size = conf->pkt_size;
    DBG_CHECK(itf->mode <= ITF_MODE_MAX);

    bool trace_list_enable = itf_name_list_match(&g_itf_trace_list,
        conf->hier_name, name);
    itf->trace_enable = trace_list_enable ||
        (dbg_get_bool_env(ITF_TRACE_ENV) && (!conf->force_disable_trace));
    bool vcd_list_enable = itf_name_list_match(&g_itf_vcd_list,
        conf->hier_name, name);
    itf->vcd_enable = vcd_list_enable || dbg_get_bool_env(ITF_VCD_ENV);
    itf->pkt2str = conf->pkt2str;
    itf->reg_vcd = conf->reg_vcd;

    if (itf->trace_enable) {
        struct stat s;
        if (stat(ITF_TRACE_DIR, &s) || !S_ISDIR(s.st_mode)) {
            mkdir(ITF_TRACE_DIR, 0755);
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

    if (itf->ctx.signal.trace_fp) {
        fclose(itf->ctx.signal.trace_fp);
        itf->ctx.signal.trace_fp = NULL;
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

    if (itf->ctx.fifo.trace_slv_fp) {
        fclose(itf->ctx.fifo.trace_slv_fp);
        itf->ctx.fifo.trace_slv_fp = NULL;
    }

    if (itf->ctx.fifo.trace_mst_fp) {
        fclose(itf->ctx.fifo.trace_mst_fp);
        itf->ctx.fifo.trace_mst_fp = NULL;
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

    if (itf->trace_enable) {
        if (itf->ctx.fifo.trace_mst_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->ctx.fifo.trace_mst_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.fifo.trace_mst_fp);
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

    if (itf->trace_enable) {
        if (itf->ctx.fifo.trace_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(pkt, pkt_str);
            fprintf(itf->ctx.fifo.trace_slv_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.fifo.trace_slv_fp);
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

    if (itf->trace_enable && itf->ctx.signal.trace_fp != NULL) {
        void *old = itf->ctx.signal.old_pkt_data;
        void *cur = itf->ctx.signal.shared_pkt_data;
        if (memcmp(cur, old, itf->pkt_size) != 0) {
            char pkt_str[1024];
            itf->pkt2str(cur, pkt_str);
            fprintf(itf->ctx.signal.trace_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.signal.trace_fp);
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

    if (itf->trace_enable) {
        if (itf->ctx.fifo.trace_slv_fp) {
            char pkt_str[1024];
            itf->pkt2str(get_fifo_pkt_addr(itf, itf->ctx.fifo.rptr), pkt_str);
            fprintf(itf->ctx.fifo.trace_slv_fp, "%"U64_HEX_LZ_FMT" %s", *itf->cycle, pkt_str);
            fflush(itf->ctx.fifo.trace_slv_fp);
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
