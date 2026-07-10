#ifndef RV32G_DEF_H
#define RV32G_DEF_H

#include <string.h>

#define KiB (1024ull)
#define MiB (1024ull * 1024ull)
#define GiB (1024ull * 1024ull * 1024ull)

#define I32_FMT "%d"
#define U32_FMT "%u"

#if defined(__linux__)
#define U64_FMT "lu"
#define U64_HEX_FMT "lx"
#define U64_HEX_LZ_FMT "016lx"
#elif defined(__APPLE__)
#define U64_FMT "llu"
#define U64_HEX_FMT "llx"
#define U64_HEX_LZ_FMT "016llx"
#endif

#define ADDR_IN(addr, base, size) (((addr) >= (base)) && ((addr) < ((base) + (size))))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline void cpu_relax(void)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("pause" ::: "memory");
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ volatile("yield" ::: "memory");
#endif
}

#endif
