#include <stdio.h>
#include "base/ost.h"
#include "utils.h"

typedef struct ost_test_ctx {
    u32 id;
    u32 data;
} ost_test_ctx_t;

typedef struct ost_tb {
    ut_sbd_t sbd;
} ost_tb_t;

static void tb_construct(ost_tb_t *tb, const char *name)
{
    (void)name;
    ut_sbd_init(&tb->sbd);
}

static void tb_free(ost_tb_t *tb)
{
    (void)tb;
}

TEST_CASE(ost_tb_t, ostq_head_order)
{
    TEST_BEGIN("OSTQ Head Order");

    ostq_t ost;
    ostq_construct(&ost, sizeof(ost_test_ctx_t), 4);

    ost_test_ctx_t in0 = { .id = 0, .data = 0x11111111 };
    ost_test_ctx_t in1 = { .id = 1, .data = 0x22222222 };
    u32 slot0;
    u32 slot1;
    REQUIRE(ostq_alloc(&ost, &in0, &slot0), "first ordered alloc succeeds");
    REQUIRE(ostq_alloc(&ost, &in1, &slot1), "second ordered alloc succeeds");
    REQUIRE(ostq_count(&ost) == 2, "count tracks two entries");

    ost_test_ctx_t out;
    u32 slot;
    REQUIRE(ostq_peek_head(&ost, &out, &slot), "peek head succeeds");
    REQUIRE(slot == slot0 && out.id == 0 && out.data == 0x11111111,
            "oldest entry is first allocation");
    ostq_free_head(&ost);
    REQUIRE(ostq_count(&ost) == 2, "free head is pending until clock");
    REQUIRE(!ostq_peek_head(&ost, &out, &slot), "pending-free head is not visible");
    ostq_clock(&ost);

    REQUIRE(ostq_peek_head(&ost, &out, &slot), "peek next head succeeds");
    REQUIRE(slot == slot1 && out.id == 1 && out.data == 0x22222222,
            "next entry is second allocation");
    ostq_free_head(&ost);
    REQUIRE(!ostq_empty(&ost), "second free is pending until clock");
    ostq_clock(&ost);
    REQUIRE(ostq_empty(&ost), "table empty after freeing both entries");

    ostq_free(&ost);
    TEST_END();
}

TEST_CASE(ost_tb_t, ostq_full_backpressure_and_reuse)
{
    TEST_BEGIN("OSTQ Full Backpressure and Reuse");

    ostq_t ost;
    ostq_construct(&ost, sizeof(ost_test_ctx_t), 2);

    ost_test_ctx_t in = { .id = 10, .data = 0xaa };
    u32 slot;
    REQUIRE(ostq_alloc(&ost, &in, NULL), "alloc entry 0 succeeds");
    in.id = 11;
    REQUIRE(ostq_alloc(&ost, &in, &slot), "alloc entry 1 succeeds");
    in.id = 12;
    REQUIRE(!ostq_alloc(&ost, &in, NULL), "alloc fails when table is full");
    REQUIRE(ostq_full(&ost), "table reports full");

    ost_test_ctx_t out;
    REQUIRE(ostq_peek_head(&ost, &out, &slot), "head available while full");
    ostq_free_head(&ost);
    REQUIRE(ostq_full(&ost), "table remains full until clock after free");
    REQUIRE(!ostq_alloc(&ost, &in, &slot), "alloc still fails before free clock");
    ostq_clock(&ost);
    REQUIRE(!ostq_full(&ost), "table not full after free clock");
    REQUIRE(ostq_alloc(&ost, &in, &slot), "alloc reuses freed head slot");
    REQUIRE(ostq_count(&ost) == 2, "count returns to depth after reuse");

    ostq_free(&ost);
    TEST_END();
}

TEST_CASE(ost_tb_t, ostq_slot_payload_update)
{
    TEST_BEGIN("OSTQ Slot Payload Update");

    ostq_t ost;
    ostq_construct(&ost, sizeof(ost_test_ctx_t), 2);

    ost_test_ctx_t in = { .id = 3, .data = 0x1234 };
    u32 slot;
    REQUIRE(ostq_alloc(&ost, &in, &slot), "alloc succeeds");

    ost_test_ctx_t upd = { .id = 4, .data = 0xabcd };
    ostq_write_slot(&ost, slot, &upd);

    ost_test_ctx_t out;
    ostq_read_slot(&ost, slot, &out);
    REQUIRE(out.id == 4 && out.data == 0xabcd, "slot payload readback reflects update");

    ostq_free_head(&ost);
    REQUIRE(!ostq_slot_valid(&ost, slot), "slot logically invalid after free request");
    REQUIRE(!ostq_empty(&ost), "slot still occupies capacity until clock");
    ostq_clock(&ost);
    REQUIRE(ostq_empty(&ost), "slot physically frees after clock");

    ostq_free(&ost);
    TEST_END();
}

TEST_CASE(ost_tb_t, ostk_key_order)
{
    TEST_BEGIN("OSTK Key Order");

    ostk_t ost;
    ostk_construct(&ost, sizeof(ost_test_ctx_t), 4);

    ost_test_ctx_t a0 = { .id = 0, .data = 0xa0 };
    ost_test_ctx_t b0 = { .id = 1, .data = 0xb0 };
    ost_test_ctx_t a1 = { .id = 2, .data = 0xa1 };
    REQUIRE(ostk_alloc(&ost, 7, &a0, NULL), "alloc key 7 first succeeds");
    REQUIRE(ostk_alloc(&ost, 9, &b0, NULL), "alloc key 9 succeeds");
    REQUIRE(ostk_alloc(&ost, 7, &a1, NULL), "alloc key 7 second succeeds");

    ost_test_ctx_t out;
    u32 slot;
    REQUIRE(ostk_peek_key(&ost, 9, &out, &slot), "peek key 9 succeeds before key 7");
    REQUIRE(out.id == 1 && out.data == 0xb0, "different key may return out of global order");
    ostk_free_slot(&ost, slot);
    REQUIRE(ostk_count(&ost) == 3, "keyed free remains pending until clock");
    REQUIRE(!ostk_peek_key(&ost, 9, &out, &slot), "pending-free key is not visible");
    ostk_clock(&ost);

    REQUIRE(ostk_peek_key(&ost, 7, &out, &slot), "peek key 7 first succeeds");
    REQUIRE(out.id == 0 && out.data == 0xa0, "same key returns first matching allocation");
    ostk_free_slot(&ost, slot);
    ostk_clock(&ost);

    REQUIRE(ostk_peek_key(&ost, 7, &out, &slot), "peek key 7 second succeeds");
    REQUIRE(out.id == 2 && out.data == 0xa1, "same key returns second matching allocation");
    ostk_free_slot(&ost, slot);
    ostk_clock(&ost);

    REQUIRE(!ostk_peek_key(&ost, 7, &out, &slot), "missing key lookup fails");
    REQUIRE(ostk_empty(&ost), "table empty after keyed frees");

    ostk_free(&ost);
    TEST_END();
}

TEST_CASE(ost_tb_t, ostk_full_backpressure_and_hole_reuse)
{
    TEST_BEGIN("OSTK Full Backpressure and Hole Reuse");

    ostk_t ost;
    ostk_construct(&ost, sizeof(ost_test_ctx_t), 3);

    ost_test_ctx_t in0 = { .id = 0, .data = 0x10 };
    ost_test_ctx_t in1 = { .id = 1, .data = 0x20 };
    ost_test_ctx_t in2 = { .id = 2, .data = 0x30 };
    ost_test_ctx_t in3 = { .id = 3, .data = 0x40 };
    REQUIRE(ostk_alloc(&ost, 1, &in0, NULL), "alloc key 1 succeeds");
    REQUIRE(ostk_alloc(&ost, 2, &in1, NULL), "alloc key 2 succeeds");
    REQUIRE(ostk_alloc(&ost, 1, &in2, NULL), "alloc key 1 second succeeds");
    REQUIRE(ostk_full(&ost), "keyed table reports full");
    REQUIRE(!ostk_alloc(&ost, 3, &in3, NULL), "keyed alloc fails while full");

    ost_test_ctx_t out;
    u32 slot;
    REQUIRE(ostk_peek_key(&ost, 2, &out, &slot), "peek middle key succeeds");
    REQUIRE(out.id == 1, "middle key payload is returned");
    ostk_free_slot(&ost, slot);
    REQUIRE(ostk_full(&ost), "keyed table remains full until clock after free");
    REQUIRE(!ostk_alloc(&ost, 3, &in3, &slot), "keyed alloc still fails before free clock");
    ostk_clock(&ost);
    REQUIRE(ostk_alloc(&ost, 3, &in3, &slot), "keyed alloc reuses freed hole");
    REQUIRE(ostk_count(&ost) == 3, "keyed count returns to depth");

    ostk_free(&ost);
    TEST_END();
}

TEST_CASE(ost_tb_t, ostk_slot_payload_update)
{
    TEST_BEGIN("OSTK Slot Payload Update");

    ostk_t ost;
    ostk_construct(&ost, sizeof(ost_test_ctx_t), 2);

    ost_test_ctx_t in = { .id = 5, .data = 0x5555 };
    u32 slot;
    REQUIRE(ostk_alloc(&ost, 4, &in, &slot), "keyed alloc succeeds");

    ost_test_ctx_t upd = { .id = 6, .data = 0x6666 };
    ostk_write_slot(&ost, slot, &upd);

    ost_test_ctx_t out;
    ostk_read_slot(&ost, slot, &out);
    REQUIRE(out.id == 6 && out.data == 0x6666, "keyed payload readback reflects update");

    ostk_free_slot(&ost, slot);
    REQUIRE(!ostk_slot_valid(&ost, slot), "keyed slot logically invalid after free request");
    REQUIRE(!ostk_empty(&ost), "keyed slot still occupies capacity until clock");
    ostk_clock(&ost);
    REQUIRE(ostk_empty(&ost), "keyed slot physically frees after clock");

    ostk_free(&ost);
    TEST_END();
}

int main(void)
{
    ost_tb_t tb;
    tb_construct(&tb, "ost_tb");

    TEST_RUN(ostq_head_order);
    TEST_RUN(ostq_full_backpressure_and_reuse);
    TEST_RUN(ostq_slot_payload_update);
    TEST_RUN(ostk_key_order);
    TEST_RUN(ostk_full_backpressure_and_hole_reuse);
    TEST_RUN(ostk_slot_payload_update);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
