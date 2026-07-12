#include <stdint.h>
#include <stdio.h>

static volatile int32_t atom_word = 5;

static int fail(const char *name, int32_t got, int32_t exp)
{
    printf("rv_atomic: FAIL %s got=%ld exp=%ld\n",
        name, (long)got, (long)exp);
    return 1;
}

static inline int32_t amo_add(volatile int32_t *addr, int32_t val)
{
    int32_t old;
    __asm__ volatile ("amoadd.w %0, %2, %1"
        : "=r"(old), "+A"(*addr)
        : "r"(val)
        : "memory");
    return old;
}

static inline int32_t amo_swap(volatile int32_t *addr, int32_t val)
{
    int32_t old;
    __asm__ volatile ("amoswap.w %0, %2, %1"
        : "=r"(old), "+A"(*addr)
        : "r"(val)
        : "memory");
    return old;
}

static inline int32_t lrsc_add(volatile int32_t *addr, int32_t val)
{
    int32_t tmp;
    int32_t fail_flag;

    __asm__ volatile (
        "0:\n"
        "lr.w %0, %2\n"
        "add %0, %0, %3\n"
        "sc.w %1, %0, %2\n"
        "bnez %1, 0b\n"
        : "=&r"(tmp), "=&r"(fail_flag), "+A"(*addr)
        : "r"(val)
        : "memory");

    return tmp;
}

int main(void)
{
    int32_t old = amo_add(&atom_word, 7);
    if (old != 5 || atom_word != 12) {
        return fail("amoadd", atom_word, 12);
    }

    old = amo_swap(&atom_word, -3);
    if (old != 12 || atom_word != -3) {
        return fail("amoswap", atom_word, -3);
    }

    int32_t new_val = lrsc_add(&atom_word, 10);
    if (new_val != 7 || atom_word != 7) {
        return fail("lrsc", atom_word, 7);
    }

    puts("rv_atomic: PASS");
    return 0;
}

