#ifndef _CPU_H
#define _CPU_H

#define NCPU 84
#define CPU_CLOCK 2.25

// Assuming a 64-byte cache line
#define CACHE_LINE_SIZE 64
#define cachepadded_int32_t alignas(CACHE_LINE_SIZE) int32_t


#endif