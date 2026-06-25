#ifndef APB_DEMUX_H
#define APB_DEMUX_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_req_if.h"
#include "itf/apb_rsp_if.h"

#define APB_DEMUX_GST_NUM_MAX 16

typedef struct apb_demux {
    itf_t *host_apb_req_slv;
    itf_t *host_apb_rsp_mst;
    itf_t *gst_apb_req_msts[APB_DEMUX_GST_NUM_MAX];
    itf_t *gst_apb_rsp_slvs[APB_DEMUX_GST_NUM_MAX];

    u32 gst_num;
    u32 gst_bases[APB_DEMUX_GST_NUM_MAX];
    u32 gst_sizes[APB_DEMUX_GST_NUM_MAX];
} apb_demux_t;

extern void apb_demux_construct(apb_demux_t *apb_demux, const char *name,
    u32 gst_num, const u32 *gst_bases, const u32 *gst_sizes);
extern void apb_demux_reset(apb_demux_t *apb_demux);
extern void apb_demux_clock(apb_demux_t *apb_demux);
extern void apb_demux_free(apb_demux_t *apb_demux);

#endif
