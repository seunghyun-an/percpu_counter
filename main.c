#define _POSIX_C_SOURCE 200112L /* Or higher */
#define _GNU_SOURCE
// Ad-hoc for pthread barrier

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>
#include <sched.h>

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <malloc.h>
#include <numa.h>
#include <numaif.h>
#include <pthread.h>

#include "percpu_counter.h"
#include "cpu.h"

// increment probability as percent
// #define INC_PROB 5
// #define DEC_PROB 3
int INC_PROB;
int DEC_PROB;

// Core pinning
int stick_this_thread_to_core(int core_id)
{
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (core_id < 0 || core_id >= num_cores)
        return EINVAL;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

// synchronization barrier for current thread counter
pthread_barrier_t g_barrier;

struct percpu_counter fbc;
int corenum[NCPU];
int finished;

void *job(void *cookie)
{
    int thread_num = (int)cookie;
    stick_this_thread_to_core(corenum[thread_num]);
    struct timespec start, current;
    int seed = 54654135;

    pthread_barrier_wait(&g_barrier);

    uint64_t elapsed_time = 0;

    unsigned long long completed = 0;
    unsigned long long completed2 = 0;
    unsigned long long completed3 = 0;
    unsigned long long completed4 = 0;
    unsigned long long freeing = 0;
    unsigned long long __tmp;

    int r;
    int depth = 0;

    // just in case of compiler optimization;
    unsigned long long _tmp = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (finished == 0)
    {
        r = rand_r(&seed);
        if (r % 100 <= INC_PROB)
        {
            completed += 1;
            percpu_counter_add(&fbc, 1, corenum[thread_num]);
            depth += 1;
        }
        else if (r % 100 <= INC_PROB + DEC_PROB)
        {
            if (depth <= 0)
                continue;

            percpu_counter_add(&fbc, -1, corenum[thread_num]);
            depth -= 1;
            __tmp = percpu_counter_read(&fbc);
            if (__tmp <= 0)
            {
                percpu_counter_sync(&fbc, thread_num);
                __tmp = percpu_counter_read(&fbc);
                if (__tmp <= 0)
                {
                    __tmp = percpu_counter_sum(&fbc);
                    if (__tmp == 0)
                        freeing++;
                }
            }
        }
        else
        {
            int __tmp = percpu_counter_read(&fbc);
            if (__tmp <= 0)
            {
                percpu_counter_sync(&fbc, thread_num);
                __tmp = percpu_counter_read(&fbc);
                if (__tmp <= 0)
                {
                    _tmp += percpu_counter_sum(&fbc);
                    completed4 += 1;
                }
                completed2 += 1;
            }
            completed3 += 1;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &current);
    elapsed_time = (current.tv_sec - start.tv_sec) * 1e9 +
                   (current.tv_nsec - start.tv_nsec);
    double elapsed_seconds = (double)elapsed_time / 1e9;

    pthread_barrier_wait(&g_barrier);

    if (elapsed_time)
        printf("Transaction %llu %llu %llu %llu, %llu completed for %.2f seconds from core %d "
               "with percpu target bandwidth %.2f MT/s\n",
               freeing, completed3, completed2, completed4, completed2 + completed, elapsed_seconds, corenum[thread_num],
               ((double)(completed + completed2 + completed3)) / elapsed_time * 1e3);
    else
        fprintf(stderr, "Err: elapsed time 0\n");

    return NULL;
}

cachepadded_int64_t atomic_test_v;

void *job2(void *cookie)
{
    int thread_num = (int)cookie;
    stick_this_thread_to_core(corenum[thread_num]);
    struct timespec start, current;

    pthread_barrier_wait(&g_barrier);
    int seed = 54654135;

    uint64_t elapsed_time = 0;

    unsigned long long completed = 0;
    unsigned long long completed2 = 0;
    unsigned long long completed3 = 0;
    unsigned long long freeing = 0;
    int r;

    // just in case of compiler optimization;
    volatile unsigned long long _tmp = 0;
    unsigned long long depth = 0;
    volatile unsigned long long __tmp;

    clock_gettime(CLOCK_MONOTONIC, &start);
    while (finished == 0)
    {
        r = rand_r(&seed);
        if (r % 100 <= INC_PROB)
        {
            depth++;
            completed += 1;
            atomic_fetch_add(&atomic_test_v.val, 1);
        }
        else if (r % 100 <= INC_PROB + DEC_PROB)
        {
            if (depth > 0)
            {
                atomic_fetch_add(&atomic_test_v.val, -1);
                depth--;
                if (atomic_load(&atomic_test_v.val) <= 0)
                {
                    freeing++;
                }
                completed += 1;
            }
        }
        else
        {
            __tmp = atomic_load(&atomic_test_v.val);
            if (__tmp <= 0)
            {
                freeing++;
            }
            completed += 1;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &current);
    elapsed_time = (current.tv_sec - start.tv_sec) * 1e9 +
                   (current.tv_nsec - start.tv_nsec);
    double elapsed_seconds = (double)elapsed_time / 1e9;

    pthread_barrier_wait(&g_barrier);

    if (elapsed_time)
        printf("Transaction %llu %llu %llu %llu completed for %.2f seconds from core %d "
               "with atomic target bandwidth %.2f MT/s\n",
               freeing, completed3, completed2, completed2 + completed, elapsed_seconds, corenum[thread_num],
               ((double)(completed + completed2 + completed3)) / elapsed_time * 1e3);
    else
        fprintf(stderr, "Err: elapsed time 0\n");

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        fprintf(stderr, "wrong argument\n");
    }

    // corenum[1] = 0;
    // corenum[2] = 7;

    atomic_test_v.val = 0;
    int nthread = atoi(argv[1]);
    int atomic = atoi(argv[2]);
    int rr_llc = atoi(argv[3]);
    INC_PROB = atoi(argv[4]);
    DEC_PROB = atoi(argv[5]);

    if (rr_llc)
    {
        int tmp[84] = {0, 7, 14, 21, 28, 35, 42, 49, 56, 63, 70, 77, 1, 8, 15, 22, 29, 36, 43, 50, 57, 64, 71, 78, 2, 9, 16, 23, 30, 37, 44, 51, 58, 65, 72, 79, 3, 10, 17, 24, 31, 38, 45, 52, 59, 66, 73, 80, 4, 11, 18, 25, 32, 39, 46, 53, 60, 67, 74, 81, 5, 12, 19, 26, 33, 40, 47, 54, 61, 68, 75, 82, 6, 13, 20, 27, 34, 41, 48, 55, 62, 69, 76, 83};
        for (int i = 0; i < NCPU; i++)
        {
            corenum[i] = tmp[i];
        }
    }
    else
    {
        for (int i = 0; i < NCPU; i++)
        {
            corenum[i] = i;
        }
    }
    int stressing_time = 1;
    finished = 0;
    percpu_counter_init(&fbc, 0);
    percpu_counter_set(&fbc, 0);
    printf("sizeof fbc : %d\n", sizeof(fbc));
    printf("sizeof percpu_counter_batch : %d\n", percpu_counter_batch);

    pthread_t thr[nthread];

    pthread_barrier_init(&g_barrier, NULL, nthread - 1);
    printf("starting benchmark\n");
    for (int p = 1; p < nthread; ++p)
    {
        if (atomic)
        {
            pthread_create(&thr[p], NULL, job2, p);
        }
        else
        {
            pthread_create(&thr[p], NULL, job, p);
        }
    }

    sleep(stressing_time);
    finished = 1;
    for (int p = 1; p < nthread; ++p)
        pthread_join(thr[p], NULL);

    pthread_barrier_destroy(&g_barrier);

    return 0;
}