#include "vcd.h"
#include <string.h>
#include "dbg/chk.h"
#include "dbg/env.h"

static dbg_vcd_t g_vcd;

static void gen_sig_token(char *token)
{
    static u64 id = 0;
    const char *digital = "abcdefghijklmnop";
    for (u32 i = 0; i < 16; i++) {
        token[i] = digital[(id >> (i * 4)) & 0xf];
    }
    token[16] = '\0';
    id++;
}

static void print_vcd_header()
{
    fprintf(g_vcd.vcd_fp,
        "$date\n"
        "    Jan 1, 2025, 00:00:00\n"
        "$end\n"

        "$version\n"
        "    RVBucket CA model VCD\n"
        "$end\n"

        "$timescale\n"
        "    1 ps\n"
        "$end\n"
    );
}

__attribute__((constructor)) void dbg_vcd_constructor()
{
    g_vcd.enable = dbg_get_bool_env("GEN_VCD");
    g_vcd.sig_list.head = NULL;
    g_vcd.sig_list.tail = NULL;
    gen_sig_token(g_vcd.clk_token);

    if (g_vcd.enable) {
        g_vcd.vcd_fp = fopen("sim.vcd", "w");
        print_vcd_header();
    } else {
        g_vcd.vcd_fp = NULL;
    }
}

void dbg_vcd_scope_begin(const char *name)
{
    if (!g_vcd.enable) {
        return;
    }
    DBG_CHECK(g_vcd.vcd_fp);

    fprintf(g_vcd.vcd_fp, "$scope module %s $end\n", name);
}

void dbg_vcd_set_clk(const u64 *cycle)
{
    g_vcd.cycle = cycle;

    if (!g_vcd.enable) {
        return;
    }
    DBG_CHECK(g_vcd.vcd_fp);

    fprintf(g_vcd.vcd_fp, "$var wire 1 %s clk $end\n", g_vcd.clk_token);
}

void dbg_vcd_add_sig(const char *name, dbg_sig_type_t type, u32 bits, const void *ref)
{
    if (!g_vcd.enable) {
        return;
    }
    DBG_CHECK(g_vcd.vcd_fp);

    dbg_sig_t *sig = malloc(sizeof(dbg_sig_t));
    sig->bits = bits;
    sig->bytes = bits / 8 + ((bits % 8) ? 1 : 0);
    sig->cur = ref;
    sig->nxt = NULL;
    memset(sig->old, 0, sizeof(sig->old));
    gen_sig_token(sig->token);

    if (g_vcd.sig_list.head == NULL) {
        DBG_CHECK(g_vcd.sig_list.tail == NULL);
        g_vcd.sig_list.head = sig;
        g_vcd.sig_list.tail = sig;
    } else {
        DBG_CHECK(g_vcd.sig_list.tail != NULL);
        g_vcd.sig_list.tail->nxt = sig;
        g_vcd.sig_list.tail = sig;
    }

    static const char *type_names[] = {
        [DBG_SIG_TYPE_WIRE] = "wire",
        [DBG_SIG_TYPE_REG] = "reg"
    };

    fprintf(g_vcd.vcd_fp, "$var %s %u %s %s $end\n", type_names[type], bits, sig->token, name);
}

void dbg_vcd_scope_end()
{
    if (!g_vcd.enable) {
        return;
    }
    DBG_CHECK(g_vcd.vcd_fp);

    fprintf(g_vcd.vcd_fp, "$upscope $end\n");
}

static void print_clk_rising()
{
    fprintf(g_vcd.vcd_fp, "#%lu\n", *g_vcd.cycle * 1000);
    fprintf(g_vcd.vcd_fp, "1%s\n", g_vcd.clk_token);
}

static void print_clk_falling()
{
    fprintf(g_vcd.vcd_fp, "#%lu\n", *g_vcd.cycle * 1000 + 500);
    fprintf(g_vcd.vcd_fp, "0%s\n", g_vcd.clk_token);
}

static void gen_sig_bin_str(const void *ref, u32 bits, u32 bytes, char *bin_str)
{
    DBG_CHECK(bits <= DBG_SIG_BIT_MAX);

    u64 sig;
    memcpy(&sig, ref, bytes);

    bool start = false;
    u32 bit_idx = 0;

    for (s32 i = bits - 1; i >= 0; i--) {
        bool bit = (sig & (1UL << i));
        if (bit) {
            start = true;
        }
        if (start) {
            bin_str[bit_idx++] = bit ? '1' : '0';
        }
    }

    if (bit_idx == 0) {
        bin_str[bit_idx++] = '0';
    }

    bin_str[bit_idx] = '\0';
}

void dbg_vcd_reset()
{
    if (!g_vcd.enable) {
        return;
    }
    DBG_CHECK(g_vcd.vcd_fp);

    fprintf(g_vcd.vcd_fp, "$enddefinitions $end\n\n");
    fprintf(g_vcd.vcd_fp, "$dumpvars\n");
    fprintf(g_vcd.vcd_fp, "1%s\n", g_vcd.clk_token);
    for (dbg_sig_t *sig = g_vcd.sig_list.head; sig != NULL; sig = sig->nxt) {
        char bin_str[DBG_SIG_BIT_MAX];
        gen_sig_bin_str(sig->cur, sig->bits, sig->bytes, bin_str);
        fprintf(g_vcd.vcd_fp, "b%s %s\n", bin_str, sig->token);
    }
    fprintf(g_vcd.vcd_fp, "$end\n");
    print_clk_falling();
}

static bool value_changed(const void *cur, const void *old, u32 bits, u32 bytes)
{
    DBG_CHECK(bits <= DBG_SIG_BIT_MAX);

    u64 cur_sig, old_sig, mask;
    mask = (~0UL) >> (DBG_SIG_BIT_MAX - bits);
    memcpy(&cur_sig, cur, bytes);
    memcpy(&old_sig, old, bytes);

    return ((cur_sig & mask) != (old_sig & mask));
}

void dbg_vcd_clock()
{
    if (!g_vcd.enable) {
        return;
    }
    DBG_CHECK(g_vcd.vcd_fp);

    print_clk_rising();
    for (dbg_sig_t *sig = g_vcd.sig_list.head; sig != NULL; sig = sig->nxt) {
        if (value_changed(sig->cur, sig->old, sig->bits, sig->bytes)) {
            char bin_str[DBG_SIG_BIT_MAX];
            gen_sig_bin_str(sig->cur, sig->bits, sig->bytes, bin_str);
            fprintf(g_vcd.vcd_fp, "b%s %s\n", bin_str, sig->token);
            memcpy(sig->old, sig->cur, sig->bytes);
        }
    }
    print_clk_falling();
}

__attribute__((destructor)) void dbg_vcd_destructor()
{
    if (!g_vcd.enable) {
        return;
    }
    DBG_CHECK(g_vcd.vcd_fp);

    dbg_sig_t *sig = g_vcd.sig_list.head;
    while (sig != NULL) {
        dbg_sig_t *tmp = sig;
        sig = sig->nxt;
        free(tmp);
    }

    g_vcd.sig_list.head = NULL;
    g_vcd.sig_list.tail = NULL;

    if (g_vcd.vcd_fp != NULL) {
        fclose(g_vcd.vcd_fp);
        g_vcd.vcd_fp = NULL;
    }
}