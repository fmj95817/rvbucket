#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef union { s8 s; u8 u; } i8;

typedef int16_t s16;
typedef uint16_t u16;
typedef union { s16 s; u16 u; } i16;

typedef int32_t s32;
typedef uint32_t u32;
typedef union { s32 s; u32 u; } i32;

typedef int64_t s64;
typedef uint64_t u64;
typedef union { s64 s; u64 u; } i64;

#define KiB (1024)
#define MiB (1024 * 1024)
#define GiB (1024 * 1024 * 1024)

#define I32_FMT "%d"
#define U32_FMT "%u"

#if defined(__linux__)
#define U64_FMT "%lu"
#elif defined(__APPLE__)
#define U64_FMT "%llu"
#endif

#define ADDR_IN(addr, base, size) (((addr) >= (base)) && ((addr) < ((base) + (size))))

#endif