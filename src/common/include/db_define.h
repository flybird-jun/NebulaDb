#pragma once
#include <assert.h>
#include <stdint.h>
typedef enum {
    NEBULA_OK = 0,
    NEBULA_FAILED_OPERATE_FILE,
    NEBULA_INVALID_ARGS,
    NEBULA_UNEXPECT_NULL,
    NEBULA_OUT_OF_MEMORY
} Status;
#ifndef NDEBUG
#define DB_ASSERT(expr) assert(expr)
#define DB_POINTER(p) assert(p != nullptr);
#define DB_POINTER2(p1, p2) assert(p1 != nullptr && p2 != nullptr);
#else
#define DB_ASSERT(expr)
#define DB_POINTER(p)
#endif
#define SIZE_K(n) (n * 1024)

// 将请求 size 向上对齐到 alignment 的整数倍(alignment 必须是 2 的幂)
static inline uint64_t align_up(uint64_t size, uint64_t alignment) 
{
    DB_ASSERT((alignment & (alignment - 1)) == 0);
    return (size + alignment - 1) & ~(alignment - 1);
}

