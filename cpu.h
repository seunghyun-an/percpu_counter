#ifndef _CPU_H
#define _CPU_H

#define NCPU 84
#define CPU_CLOCK 2.25

// Assuming a 64-byte cache line
#define CACHE_LINE_SIZE 256

#define cachepadded_int64_t __cachepadded_int64_t

#include <stdint.h>

typedef union __cachepadded_int64 {
    _Atomic int64_t val;
    char padding[CACHE_LINE_SIZE];
} __cachepadded_int64_t;


#endif