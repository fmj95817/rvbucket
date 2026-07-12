#include <stdint.h>
#include <stdio.h>

typedef uint32_t (*op_fn_t)(uint32_t, uint32_t);

static int fail(const char *name, uint32_t got, uint32_t exp)
{
    printf("rv_indirect_call: FAIL %s got=0x%08x exp=0x%08x\n",
        name, got, exp);
    return 1;
}

__attribute__((noinline))
static uint32_t op_add(uint32_t a, uint32_t b)
{
    return a + b + 0x01020304u;
}

__attribute__((noinline))
static uint32_t op_xor(uint32_t a, uint32_t b)
{
    return a ^ (b << 7) ^ (b >> 3);
}

__attribute__((noinline))
static uint32_t op_mix(uint32_t a, uint32_t b)
{
    return (a * 33u) + (b ^ 0xa5a5a5a5u);
}

__attribute__((noinline))
static uint32_t op_rot(uint32_t a, uint32_t b)
{
    uint32_t n = b & 31u;
    return ((a << n) | (a >> ((32u - n) & 31u))) + 0x7f4a7c15u;
}

__attribute__((noinline))
static uint32_t tiny_rec(uint32_t n)
{
    if (n < 2u) {
        return n + 1u;
    }

    return tiny_rec(n - 1u) + 2u * tiny_rec(n - 2u) + 1u;
}

static uint32_t dispatch_mix(void)
{
    static op_fn_t ops[] = {
        op_add,
        op_xor,
        op_mix,
        op_rot
    };
    uint32_t acc = 0x2468ace0u;

    for (uint32_t i = 0; i < 160u; i++) {
        uint32_t arg = i * 0x9e3779b9u;
        acc = ops[(i ^ acc) & 3u](acc, arg);
    }

    return acc;
}

int main(void)
{
    uint32_t mix = dispatch_mix();
    if (mix != 0x9cd7b89eu) {
        return fail("dispatch_mix", mix, 0x9cd7b89eu);
    }

    uint32_t rec = tiny_rec(10u);
    if (rec != 1365u) {
        return fail("tiny_rec", rec, 1365u);
    }

    puts("rv_indirect_call: PASS");
    return 0;
}

