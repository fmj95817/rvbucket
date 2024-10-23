#include "lsu.h"
#include "dbg.h"

extern void lsu_construct(lsu_t *lsu, biu_t *biu)
{
    lsu->biu = biu;
}

void lsu_reset(lsu_t *lsu) {}

void lsu_free(lsu_t *lsu) {}

void lsu_ldst_trans_handler(lsu_t *lsu, ldst_if_t *i)
{
    DBG_CHECK(lsu->biu);
    biu_ldst_trans_handler(lsu->biu, i);
}