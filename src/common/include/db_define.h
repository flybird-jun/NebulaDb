#pragma once
#include <assert.h>
typedef enum {
    NEBULA_OK = 0
} Status;
#ifndef NDEBUG
#define DB_ASSERT(expr) assert(expr)
#else

#endif
