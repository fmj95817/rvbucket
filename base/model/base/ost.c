#include "ost.h"
#include <string.h>
#include "dbg/chk.h"

static void ost_entry_ctx_construct(void **entry_ctx, u32 ctx_size)
{
    DBG_CHECK(entry_ctx != NULL);
    if (ctx_size == 0) {
        *entry_ctx = NULL;
        return;
    }

    *entry_ctx = malloc(ctx_size);
    DBG_CHECK(*entry_ctx != NULL);
    memset(*entry_ctx, 0, ctx_size);
}

static void ost_entry_ctx_free(void **entry_ctx)
{
    DBG_CHECK(entry_ctx != NULL);
    if (*entry_ctx != NULL) {
        free(*entry_ctx);
        *entry_ctx = NULL;
    }
}

static void ost_copy_to_slot(void *entry_ctx, u32 ctx_size, const void *ctx)
{
    if (ctx_size == 0) {
        return;
    }
    DBG_CHECK(entry_ctx != NULL);
    DBG_CHECK(ctx != NULL);
    memcpy(entry_ctx, ctx, ctx_size);
}

static void ost_copy_from_slot(const void *entry_ctx, u32 ctx_size, void *ctx)
{
    if (ctx_size == 0) {
        return;
    }
    DBG_CHECK(entry_ctx != NULL);
    DBG_CHECK(ctx != NULL);
    memcpy(ctx, entry_ctx, ctx_size);
}

static void ost_clear_slot_ctx(void *entry_ctx, u32 ctx_size)
{
    if (ctx_size == 0) {
        return;
    }
    DBG_CHECK(entry_ctx != NULL);
    memset(entry_ctx, 0, ctx_size);
}

void ostq_construct(ostq_t *ost, u32 ctx_size, u32 depth)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(depth > 0);

    ost->depth = depth;
    ost->ctx_size = ctx_size;
    ost->entries = malloc(sizeof(*ost->entries) * depth);
    DBG_CHECK(ost->entries != NULL);
    for (u32 i = 0; i < depth; i++) {
        ost->entries[i].vld = false;
        ost->entries[i].free_pending = false;
        ost_entry_ctx_construct(&ost->entries[i].ctx, ctx_size);
    }

    ostq_reset(ost);
}

void ostq_reset(ostq_t *ost)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(ost->entries != NULL);

    for (u32 i = 0; i < ost->depth; i++) {
        ost->entries[i].vld = false;
        ost->entries[i].free_pending = false;
        ost_clear_slot_ctx(ost->entries[i].ctx, ost->ctx_size);
    }
    ost->count = 0;
    ost->rptr = 0;
    ost->wptr = 0;
}

void ostq_clock(ostq_t *ost)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(ost->entries != NULL);

    while (ost->count > 0) {
        ostq_entry_t *entry = &ost->entries[ost->rptr];
        DBG_CHECK(entry->vld);
        if (!entry->free_pending) {
            return;
        }

        entry->vld = false;
        entry->free_pending = false;
        ost_clear_slot_ctx(entry->ctx, ost->ctx_size);
        ost->count--;
        ost->rptr = (ost->rptr + 1u) % ost->depth;
    }
}

void ostq_free(ostq_t *ost)
{
    DBG_CHECK(ost != NULL);

    if (ost->entries != NULL) {
        for (u32 i = 0; i < ost->depth; i++) {
            ost_entry_ctx_free(&ost->entries[i].ctx);
        }
        free(ost->entries);
        ost->entries = NULL;
    }
    ost->depth = 0;
    ost->ctx_size = 0;
    ost->count = 0;
    ost->rptr = 0;
    ost->wptr = 0;
}

bool ostq_alloc(ostq_t *ost, const void *ctx, u32 *slot)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(ost->entries != NULL);

    if (ostq_full(ost)) {
        return false;
    }

    u32 idx = ost->wptr;
    DBG_CHECK(!ost->entries[idx].vld);
    ost->entries[idx].vld = true;
    ost->entries[idx].free_pending = false;
    ost_copy_to_slot(ost->entries[idx].ctx, ost->ctx_size, ctx);

    ost->count++;
    ost->wptr = (idx + 1u) % ost->depth;

    if (slot != NULL) {
        *slot = idx;
    }
    return true;
}

bool ostq_peek_head(ostq_t *ost, void *ctx, u32 *slot)
{
    DBG_CHECK(ost != NULL);

    if (ostq_empty(ost)) {
        return false;
    }

    DBG_CHECK(ost->entries[ost->rptr].vld);
    if (ost->entries[ost->rptr].free_pending) {
        return false;
    }
    ost_copy_from_slot(ost->entries[ost->rptr].ctx, ost->ctx_size, ctx);
    if (slot != NULL) {
        *slot = ost->rptr;
    }
    return true;
}

void ostq_free_head(ostq_t *ost)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(!ostq_empty(ost));
    DBG_CHECK(ost->entries[ost->rptr].vld);
    DBG_CHECK(!ost->entries[ost->rptr].free_pending);

    ost->entries[ost->rptr].free_pending = true;
}

void ostq_read_slot(ostq_t *ost, u32 slot, void *ctx)
{
    DBG_CHECK(ostq_slot_valid(ost, slot));
    ost_copy_from_slot(ost->entries[slot].ctx, ost->ctx_size, ctx);
}

void ostq_write_slot(ostq_t *ost, u32 slot, const void *ctx)
{
    DBG_CHECK(ostq_slot_valid(ost, slot));
    ost_copy_to_slot(ost->entries[slot].ctx, ost->ctx_size, ctx);
}

bool ostq_full(const ostq_t *ost)
{
    DBG_CHECK(ost != NULL);
    return ost->count == ost->depth;
}

bool ostq_empty(const ostq_t *ost)
{
    DBG_CHECK(ost != NULL);
    return ost->count == 0;
}

u32 ostq_count(const ostq_t *ost)
{
    DBG_CHECK(ost != NULL);
    return ost->count;
}

bool ostq_slot_valid(const ostq_t *ost, u32 slot)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(slot < ost->depth);
    return ost->entries[slot].vld && !ost->entries[slot].free_pending;
}

static bool ostk_find_free_slot(const ostk_t *ost, u32 *slot)
{
    if (ostk_full(ost)) {
        return false;
    }

    for (u32 i = 0; i < ost->depth; i++) {
        u32 idx = (ost->wptr + i) % ost->depth;
        if (!ost->entries[idx].vld) {
            *slot = idx;
            return true;
        }
    }

    DBG_CHECK(false);
    return false;
}

static bool ostk_find_head_slot(const ostk_t *ost, u32 *slot)
{
    bool found = false;
    u32 best_slot = 0;
    u64 best_seq = 0;

    for (u32 i = 0; i < ost->depth; i++) {
        const ostk_entry_t *entry = &ost->entries[i];
        if (!entry->vld || entry->free_pending) {
            continue;
        }
        if (!found || entry->issue_seq < best_seq) {
            found = true;
            best_slot = i;
            best_seq = entry->issue_seq;
        }
    }

    if (found) {
        *slot = best_slot;
    }
    return found;
}

static void ostk_update_rptr(ostk_t *ost)
{
    u32 slot;
    if (ostk_find_head_slot(ost, &slot)) {
        ost->rptr = slot;
    } else {
        ost->rptr = ost->wptr;
    }
}

void ostk_construct(ostk_t *ost, u32 ctx_size, u32 depth)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(depth > 0);

    ost->depth = depth;
    ost->ctx_size = ctx_size;
    ost->entries = malloc(sizeof(*ost->entries) * depth);
    DBG_CHECK(ost->entries != NULL);
    for (u32 i = 0; i < depth; i++) {
        ost->entries[i].vld = false;
        ost->entries[i].free_pending = false;
        ost->entries[i].key = 0;
        ost->entries[i].issue_seq = 0;
        ost_entry_ctx_construct(&ost->entries[i].ctx, ctx_size);
    }

    ostk_reset(ost);
}

void ostk_reset(ostk_t *ost)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(ost->entries != NULL);

    for (u32 i = 0; i < ost->depth; i++) {
        ost->entries[i].vld = false;
        ost->entries[i].free_pending = false;
        ost->entries[i].key = 0;
        ost->entries[i].issue_seq = 0;
        ost_clear_slot_ctx(ost->entries[i].ctx, ost->ctx_size);
    }
    ost->count = 0;
    ost->wptr = 0;
    ost->rptr = 0;
    ost->next_issue_seq = 0;
}

void ostk_clock(ostk_t *ost)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(ost->entries != NULL);

    bool freed = false;
    for (u32 i = 0; i < ost->depth; i++) {
        ostk_entry_t *entry = &ost->entries[i];
        if (!entry->vld || !entry->free_pending) {
            continue;
        }

        entry->vld = false;
        entry->free_pending = false;
        entry->key = 0;
        entry->issue_seq = 0;
        ost_clear_slot_ctx(entry->ctx, ost->ctx_size);
        DBG_CHECK(ost->count > 0);
        ost->count--;
        freed = true;
    }

    if (freed) {
        ostk_update_rptr(ost);
    }
}

void ostk_free(ostk_t *ost)
{
    DBG_CHECK(ost != NULL);

    if (ost->entries != NULL) {
        for (u32 i = 0; i < ost->depth; i++) {
            ost_entry_ctx_free(&ost->entries[i].ctx);
        }
        free(ost->entries);
        ost->entries = NULL;
    }
    ost->depth = 0;
    ost->ctx_size = 0;
    ost->count = 0;
    ost->wptr = 0;
    ost->rptr = 0;
    ost->next_issue_seq = 0;
}

bool ostk_alloc(ostk_t *ost, u32 key, const void *ctx, u32 *slot)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(ost->entries != NULL);

    u32 alloc_slot;
    if (!ostk_find_free_slot(ost, &alloc_slot)) {
        return false;
    }

    ostk_entry_t *entry = &ost->entries[alloc_slot];
    entry->vld = true;
    entry->free_pending = false;
    entry->key = key;
    entry->issue_seq = ost->next_issue_seq++;
    ost_copy_to_slot(entry->ctx, ost->ctx_size, ctx);

    ost->count++;
    ost->wptr = (alloc_slot + 1u) % ost->depth;
    ostk_update_rptr(ost);

    if (slot != NULL) {
        *slot = alloc_slot;
    }
    return true;
}

bool ostk_peek_key(ostk_t *ost, u32 key, void *ctx, u32 *slot)
{
    DBG_CHECK(ost != NULL);

    bool found = false;
    u32 best_slot = 0;
    u64 best_seq = 0;

    for (u32 i = 0; i < ost->depth; i++) {
        const ostk_entry_t *entry = &ost->entries[i];
        if (!entry->vld || entry->free_pending || entry->key != key) {
            continue;
        }
        if (!found || entry->issue_seq < best_seq) {
            found = true;
            best_slot = i;
            best_seq = entry->issue_seq;
        }
    }

    if (!found) {
        return false;
    }

    ost_copy_from_slot(ost->entries[best_slot].ctx, ost->ctx_size, ctx);
    if (slot != NULL) {
        *slot = best_slot;
    }
    return true;
}

void ostk_free_slot(ostk_t *ost, u32 slot)
{
    DBG_CHECK(ostk_slot_valid(ost, slot));

    ost->entries[slot].free_pending = true;
}

void ostk_read_slot(ostk_t *ost, u32 slot, void *ctx)
{
    DBG_CHECK(ostk_slot_valid(ost, slot));
    ost_copy_from_slot(ost->entries[slot].ctx, ost->ctx_size, ctx);
}

void ostk_write_slot(ostk_t *ost, u32 slot, const void *ctx)
{
    DBG_CHECK(ostk_slot_valid(ost, slot));
    ost_copy_to_slot(ost->entries[slot].ctx, ost->ctx_size, ctx);
}

bool ostk_full(const ostk_t *ost)
{
    DBG_CHECK(ost != NULL);
    return ost->count == ost->depth;
}

bool ostk_empty(const ostk_t *ost)
{
    DBG_CHECK(ost != NULL);
    return ost->count == 0;
}

u32 ostk_count(const ostk_t *ost)
{
    DBG_CHECK(ost != NULL);
    return ost->count;
}

bool ostk_slot_valid(const ostk_t *ost, u32 slot)
{
    DBG_CHECK(ost != NULL);
    DBG_CHECK(slot < ost->depth);
    return ost->entries[slot].vld && !ost->entries[slot].free_pending;
}
