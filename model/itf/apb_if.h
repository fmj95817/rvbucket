#ifndef APB_IF_H
#define APB_IF_H

#include "apb_req_if.h"
#include "apb_rsp_if.h"

#define APB_IF_CONSTRUCT(module, pfx) do { \
    APB_REQ_IF_CONSTRUCT(module, pfx##apb_req_itf, 1); \
    APB_RSP_IF_CONSTRUCT(module, pfx##apb_rsp_itf, 1); \
} while (0)

#define APB_IF_FREE(obj, pfx) do { \
    itf_free(&((obj)->pfx##apb_req_itf)); \
    itf_free(&((obj)->pfx##apb_rsp_itf)); \
} while (0)

#define APB_IF_DBG_CLOCK(obj, pfx) do { \
    itf_dbg_clock(&((obj)->pfx##apb_req_itf)); \
    itf_dbg_clock(&((obj)->pfx##apb_rsp_itf)); \
} while (0)

#define APB_IF_DECL(pfx) \
    itf_t pfx##apb_req_itf; \
    itf_t pfx##apb_rsp_itf

#define APB_MST_DECL(pfx) \
    itf_t *pfx##apb_req_mst; \
    itf_t *pfx##apb_rsp_slv

#define APB_MST_ARR_DECL(pfx, size) \
    itf_t *pfx##apb_req_msts[size]; \
    itf_t *pfx##apb_rsp_slvs[size]

#define APB_SLV_DECL(pfx) \
    itf_t *pfx##apb_req_slv; \
    itf_t *pfx##apb_rsp_mst

#define APB_MST_CONNECT(mst_obj, mst_pfx, itf_obj, itf_pfx) do { \
    (mst_obj)->mst_pfx##apb_req_mst = &((itf_obj)->itf_pfx##apb_req_itf); \
    (mst_obj)->mst_pfx##apb_rsp_slv = &((itf_obj)->itf_pfx##apb_rsp_itf); \
} while (0)

#define APB_SLV_CONNECT(slv_obj, slv_pfx, itf_obj, itf_pfx) do { \
    (slv_obj)->slv_pfx##apb_req_slv = &((itf_obj)->itf_pfx##apb_req_itf); \
    (slv_obj)->slv_pfx##apb_rsp_mst = &((itf_obj)->itf_pfx##apb_rsp_itf); \
} while (0)

#define APB_MST_ARR_CONNECT(mst_obj, mst_pfx, idx, itf_obj, itf_pfx) do { \
    (mst_obj)->mst_pfx##apb_req_msts[idx] = &((itf_obj)->itf_pfx##apb_req_itf); \
    (mst_obj)->mst_pfx##apb_rsp_slvs[idx] = &((itf_obj)->itf_pfx##apb_rsp_itf); \
} while (0)

#define APB_MST_IMPORT(inner_obj, inner_pfx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##apb_req_mst = (ext_obj)->ext_pfx##apb_req_mst; \
    (inner_obj)->inner_pfx##apb_rsp_slv = (ext_obj)->ext_pfx##apb_rsp_slv; \
} while (0)

#define APB_MST_ARR_IMPORT(inner_obj, inner_pfx, idx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##apb_req_msts[idx] = (ext_obj)->ext_pfx##apb_req_mst; \
    (inner_obj)->inner_pfx##apb_rsp_slvs[idx] = (ext_obj)->ext_pfx##apb_rsp_slv; \
} while (0)

#define APB_SLV_IMPORT(inner_obj, inner_pfx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##apb_req_slv = (ext_obj)->ext_pfx##apb_req_slv; \
    (inner_obj)->inner_pfx##apb_rsp_mst = (ext_obj)->ext_pfx##apb_rsp_mst; \
} while (0)

#endif
