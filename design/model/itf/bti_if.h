#ifndef BTI_IF_H
#define BTI_IF_H

#include "bti_req_if.h"
#include "bti_rsp_if.h"

#define BTI_IF_CONSTRUCT(module, pfx, depth) do { \
    BTI_REQ_IF_CONSTRUCT(module, pfx##bti_req_itf, depth); \
    BTI_RSP_IF_CONSTRUCT(module, pfx##bti_rsp_itf, depth); \
} while (0)

#define BTI_IF_RESET(obj, pfx) do { \
    itf_reset(&((obj)->pfx##bti_req_itf)); \
    itf_reset(&((obj)->pfx##bti_rsp_itf)); \
} while (0)

#define BTI_IF_FREE(obj, pfx) do { \
    itf_free(&((obj)->pfx##bti_req_itf)); \
    itf_free(&((obj)->pfx##bti_rsp_itf)); \
} while (0)

#define BTI_IF_DBG_CLOCK(obj, pfx) do { \
    itf_dbg_clock(&((obj)->pfx##bti_req_itf)); \
    itf_dbg_clock(&((obj)->pfx##bti_rsp_itf)); \
} while (0)

#define BTI_IF_DECL(pfx) \
    itf_t pfx##bti_req_itf; \
    itf_t pfx##bti_rsp_itf

#define BTI_MST_DECL(pfx) \
    itf_t *pfx##bti_req_mst; \
    itf_t *pfx##bti_rsp_slv

#define BTI_MST_ARR_DECL(pfx, size) \
    itf_t *pfx##bti_req_msts[size]; \
    itf_t *pfx##bti_rsp_slvs[size]

#define BTI_SLV_DECL(pfx) \
    itf_t *pfx##bti_req_slv; \
    itf_t *pfx##bti_rsp_mst

#define BTI_SLV_ARR_DECL(pfx, size) \
    itf_t *pfx##bti_req_slvs[size]; \
    itf_t *pfx##bti_rsp_msts[size]

#define BTI_MST_CONNECT(mst_obj, mst_pfx, itf_obj, itf_pfx) do { \
    (mst_obj)->mst_pfx##bti_req_mst = &((itf_obj)->itf_pfx##bti_req_itf); \
    (mst_obj)->mst_pfx##bti_rsp_slv = &((itf_obj)->itf_pfx##bti_rsp_itf); \
} while (0)

#define BTI_SLV_CONNECT(slv_obj, slv_pfx, itf_obj, itf_pfx) do { \
    (slv_obj)->slv_pfx##bti_req_slv = &((itf_obj)->itf_pfx##bti_req_itf); \
    (slv_obj)->slv_pfx##bti_rsp_mst = &((itf_obj)->itf_pfx##bti_rsp_itf); \
} while (0)

#define BTI_MST_ARR_CONNECT(mst_obj, mst_pfx, idx, itf_obj, itf_pfx) do { \
    (mst_obj)->mst_pfx##bti_req_msts[idx] = &((itf_obj)->itf_pfx##bti_req_itf); \
    (mst_obj)->mst_pfx##bti_rsp_slvs[idx] = &((itf_obj)->itf_pfx##bti_rsp_itf); \
} while (0)

#define BTI_SLV_ARR_CONNECT(slv_obj, slv_pfx, idx, itf_obj, itf_pfx) do { \
    (slv_obj)->slv_pfx##bti_req_slvs[idx] = &((itf_obj)->itf_pfx##bti_req_itf); \
    (slv_obj)->slv_pfx##bti_rsp_msts[idx] = &((itf_obj)->itf_pfx##bti_rsp_itf); \
} while (0)

#define BTI_MST_IMPORT(inner_obj, inner_pfx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##bti_req_mst = (ext_obj)->ext_pfx##bti_req_mst; \
    (inner_obj)->inner_pfx##bti_rsp_slv = (ext_obj)->ext_pfx##bti_rsp_slv; \
} while (0)

#define BTI_MST_ARR_IMPORT(inner_obj, inner_pfx, idx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##bti_req_msts[idx] = (ext_obj)->ext_pfx##bti_req_mst; \
    (inner_obj)->inner_pfx##bti_rsp_slvs[idx] = (ext_obj)->ext_pfx##bti_rsp_slv; \
} while (0)

#define BTI_SLV_IMPORT(inner_obj, inner_pfx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##bti_req_slv = (ext_obj)->ext_pfx##bti_req_slv; \
    (inner_obj)->inner_pfx##bti_rsp_mst = (ext_obj)->ext_pfx##bti_rsp_mst; \
} while (0)

#define BTI_SLV_ARR_IMPORT(inner_obj, inner_pfx, idx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##bti_req_slvs[idx] = (ext_obj)->ext_pfx##bti_req_slv; \
    (inner_obj)->inner_pfx##bti_rsp_msts[idx] = (ext_obj)->ext_pfx##bti_rsp_mst; \
} while (0)

#endif
