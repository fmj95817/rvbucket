#ifndef RV32G_DEF_H
#define RV32G_DEF_H

#define KiB (1024u)
#define MiB (1024u * 1024u)
#define GiB (1024u * 1024u * 1024u)

#define I32_FMT "%d"
#define U32_FMT "%u"

#if defined(__linux__)
#define U64_FMT "%lu"
#elif defined(__APPLE__)
#define U64_FMT "%llu"
#endif

#define ADDR_IN(addr, base, size) (((addr) >= (base)) && ((addr) < ((base) + (size))))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif