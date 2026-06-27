#ifndef AXI4_IF_H
#define AXI4_IF_H

#include "axi4_aw_if.h"
#include "axi4_w_if.h"
#include "axi4_b_if.h"
#include "axi4_ar_if.h"
#include "axi4_r_if.h"

#define AXI4_IF_CONSTRUCT(module, pfx) do { \
    AXI4_AW_IF_CONSTRUCT(module, pfx##axi4_aw_itf, 1); \
    AXI4_W_IF_CONSTRUCT(module, pfx##axi4_w_itf, 1); \
    AXI4_B_IF_CONSTRUCT(module, pfx##axi4_b_itf, 1); \
    AXI4_AR_IF_CONSTRUCT(module, pfx##axi4_ar_itf, 1); \
    AXI4_R_IF_CONSTRUCT(module, pfx##axi4_r_itf, 1); \
} while (0)

#define AXI4_IF_FREE(obj, pfx) do { \
    itf_free(&((obj)->pfx##axi4_aw_itf)); \
    itf_free(&((obj)->pfx##axi4_w_itf)); \
    itf_free(&((obj)->pfx##axi4_b_itf)); \
    itf_free(&((obj)->pfx##axi4_ar_itf)); \
    itf_free(&((obj)->pfx##axi4_r_itf)); \
} while (0)

#define AXI4_IF_DBG_CLOCK(obj, pfx) do { \
    itf_dbg_clock(&((obj)->pfx##axi4_aw_itf)); \
    itf_dbg_clock(&((obj)->pfx##axi4_w_itf)); \
    itf_dbg_clock(&((obj)->pfx##axi4_b_itf)); \
    itf_dbg_clock(&((obj)->pfx##axi4_ar_itf)); \
    itf_dbg_clock(&((obj)->pfx##axi4_r_itf)); \
} while (0)

#define AXI4_IF_DECL(pfx) \
    itf_t pfx##axi4_aw_itf; \
    itf_t pfx##axi4_w_itf; \
    itf_t pfx##axi4_b_itf; \
    itf_t pfx##axi4_ar_itf; \
    itf_t pfx##axi4_r_itf

#define AXI4_MST_DECL(pfx) \
    itf_t *pfx##axi4_aw_mst; \
    itf_t *pfx##axi4_w_mst; \
    itf_t *pfx##axi4_b_slv; \
    itf_t *pfx##axi4_ar_mst; \
    itf_t *pfx##axi4_r_slv

#define AXI4_MST_ARR_DECL(pfx, size) \
    itf_t *pfx##axi4_aw_msts[size]; \
    itf_t *pfx##axi4_w_msts[size]; \
    itf_t *pfx##axi4_b_slvs[size]; \
    itf_t *pfx##axi4_ar_msts[size]; \
    itf_t *pfx##axi4_r_slvs[size]

#define AXI4_SLV_DECL(pfx) \
    itf_t *pfx##axi4_aw_slv; \
    itf_t *pfx##axi4_w_slv; \
    itf_t *pfx##axi4_b_mst; \
    itf_t *pfx##axi4_ar_slv; \
    itf_t *pfx##axi4_r_mst

#define AXI4_SLV_ARR_DECL(pfx, size) \
    itf_t *pfx##axi4_aw_slvs[size]; \
    itf_t *pfx##axi4_w_slvs[size]; \
    itf_t *pfx##axi4_b_msts[size]; \
    itf_t *pfx##axi4_ar_slvs[size]; \
    itf_t *pfx##axi4_r_msts[size]

#define AXI4_MST_CONNECT(mst_obj, mst_pfx, itf_obj, itf_pfx) do { \
    (mst_obj)->mst_pfx##axi4_aw_mst = &((itf_obj)->itf_pfx##axi4_aw_itf); \
    (mst_obj)->mst_pfx##axi4_w_mst = &((itf_obj)->itf_pfx##axi4_w_itf); \
    (mst_obj)->mst_pfx##axi4_b_slv = &((itf_obj)->itf_pfx##axi4_b_itf); \
    (mst_obj)->mst_pfx##axi4_ar_mst = &((itf_obj)->itf_pfx##axi4_ar_itf); \
    (mst_obj)->mst_pfx##axi4_r_slv = &((itf_obj)->itf_pfx##axi4_r_itf); \
} while (0)

#define AXI4_SLV_CONNECT(slv_obj, slv_pfx, itf_obj, itf_pfx) do { \
    (slv_obj)->slv_pfx##axi4_aw_slv = &((itf_obj)->itf_pfx##axi4_aw_itf); \
    (slv_obj)->slv_pfx##axi4_w_slv = &((itf_obj)->itf_pfx##axi4_w_itf); \
    (slv_obj)->slv_pfx##axi4_b_mst = &((itf_obj)->itf_pfx##axi4_b_itf); \
    (slv_obj)->slv_pfx##axi4_ar_slv = &((itf_obj)->itf_pfx##axi4_ar_itf); \
    (slv_obj)->slv_pfx##axi4_r_mst = &((itf_obj)->itf_pfx##axi4_r_itf); \
} while (0)

#define AXI4_MST_ARR_CONNECT(mst_obj, mst_pfx, idx, itf_obj, itf_pfx) do { \
    (mst_obj)->mst_pfx##axi4_aw_msts[idx] = &((itf_obj)->itf_pfx##axi4_aw_itf); \
    (mst_obj)->mst_pfx##axi4_w_msts[idx] = &((itf_obj)->itf_pfx##axi4_w_itf); \
    (mst_obj)->mst_pfx##axi4_b_slvs[idx] = &((itf_obj)->itf_pfx##axi4_b_itf); \
    (mst_obj)->mst_pfx##axi4_ar_msts[idx] = &((itf_obj)->itf_pfx##axi4_ar_itf); \
    (mst_obj)->mst_pfx##axi4_r_slvs[idx] = &((itf_obj)->itf_pfx##axi4_r_itf); \
} while (0)

#define AXI4_SLV_ARR_CONNECT(slv_obj, slv_pfx, idx, itf_obj, itf_pfx) do { \
    (slv_obj)->slv_pfx##axi4_aw_slvs[idx] = &((itf_obj)->itf_pfx##axi4_aw_itf); \
    (slv_obj)->slv_pfx##axi4_w_slvs[idx] = &((itf_obj)->itf_pfx##axi4_w_itf); \
    (slv_obj)->slv_pfx##axi4_b_msts[idx] = &((itf_obj)->itf_pfx##axi4_b_itf); \
    (slv_obj)->slv_pfx##axi4_ar_slvs[idx] = &((itf_obj)->itf_pfx##axi4_ar_itf); \
    (slv_obj)->slv_pfx##axi4_r_msts[idx] = &((itf_obj)->itf_pfx##axi4_r_itf); \
} while (0)

#define AXI4_MST_IMPORT(inner_obj, inner_pfx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##axi4_aw_mst = (ext_obj)->ext_pfx##axi4_aw_mst; \
    (inner_obj)->inner_pfx##axi4_w_mst = (ext_obj)->ext_pfx##axi4_w_mst; \
    (inner_obj)->inner_pfx##axi4_b_slv = (ext_obj)->ext_pfx##axi4_b_slv; \
    (inner_obj)->inner_pfx##axi4_ar_mst = (ext_obj)->ext_pfx##axi4_ar_mst; \
    (inner_obj)->inner_pfx##axi4_r_slv = (ext_obj)->ext_pfx##axi4_r_slv; \
} while (0)

#define AXI4_MST_ARR_IMPORT(inner_obj, inner_pfx, idx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##axi4_aw_msts[idx] = (ext_obj)->ext_pfx##axi4_aw_mst; \
    (inner_obj)->inner_pfx##axi4_w_msts[idx] = (ext_obj)->ext_pfx##axi4_w_mst; \
    (inner_obj)->inner_pfx##axi4_b_slvs[idx] = (ext_obj)->ext_pfx##axi4_b_slv; \
    (inner_obj)->inner_pfx##axi4_ar_msts[idx] = (ext_obj)->ext_pfx##axi4_ar_mst; \
    (inner_obj)->inner_pfx##axi4_r_slvs[idx] = (ext_obj)->ext_pfx##axi4_r_slv; \
} while (0)

#define AXI4_SLV_IMPORT(inner_obj, inner_pfx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##axi4_aw_slv = (ext_obj)->ext_pfx##axi4_aw_slv; \
    (inner_obj)->inner_pfx##axi4_w_slv = (ext_obj)->ext_pfx##axi4_w_slv; \
    (inner_obj)->inner_pfx##axi4_b_mst = (ext_obj)->ext_pfx##axi4_b_mst; \
    (inner_obj)->inner_pfx##axi4_ar_slv = (ext_obj)->ext_pfx##axi4_ar_slv; \
    (inner_obj)->inner_pfx##axi4_r_mst = (ext_obj)->ext_pfx##axi4_r_mst; \
} while (0)

#define AXI4_SLV_ARR_IMPORT(inner_obj, inner_pfx, idx, ext_obj, ext_pfx) do { \
    (inner_obj)->inner_pfx##axi4_aw_slvs[idx] = (ext_obj)->ext_pfx##axi4_aw_slv; \
    (inner_obj)->inner_pfx##axi4_w_slvs[idx] = (ext_obj)->ext_pfx##axi4_w_slv; \
    (inner_obj)->inner_pfx##axi4_b_msts[idx] = (ext_obj)->ext_pfx##axi4_b_mst; \
    (inner_obj)->inner_pfx##axi4_ar_slvs[idx] = (ext_obj)->ext_pfx##axi4_ar_slv; \
    (inner_obj)->inner_pfx##axi4_r_msts[idx] = (ext_obj)->ext_pfx##axi4_r_mst; \
} while (0)

#endif