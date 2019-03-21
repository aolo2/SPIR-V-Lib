#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef float  f32;
typedef double f64;

#ifndef RELEASE
#define ASSERT(expr) {\
    if (!(expr)) {\
        fprintf(stderr, "\n[ASSERT] %s:%d\n\n", __FILE__, __LINE__);\
        exit(1);\
    }\
}
#else
#define ASSERT(expr)
#endif

#define NOTIMPLEMENTED ASSERT(false)