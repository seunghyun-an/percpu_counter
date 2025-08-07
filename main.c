#define _POSIX_C_SOURCE 200112L /* Or higher */
// Ad-hoc for pthread barrier

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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


// increment probability as percent
#define INC_PROB 5


// error writers
#define ERR(x)                       \
    do                               \
    {                                \
        std::cerr << x << std::endl; \
    } while (0)
#define ERRX(x)                   \
    do                            \
    {                             \
        (std::cerr << x).flush(); \
    } while (0)



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

void *job(void *cookie){
    // this weirdness is because (void*) cannot be cast to int and back.
    int thread_num = *((int *)cookie);
    stick_this_thread_to_core(corenum[thread_num]);
    struct timespec start, current;

    pthread_barrier_wait(&g_barrier);
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64_t elapsed_time = 0;

    unsigned long long completed = 0;
    while (finished == 0)
    {

    }
    clock_gettime(CLOCK_MONOTONIC, &current);
    elapsed_time = (current.tv_sec - start.tv_sec) * 1e9 +
                       (current.tv_nsec - start.tv_nsec);
    double elapsed_seconds = (double)elapsed_time / 1e9;
    pthread_barrier_wait(&g_barrier);
    printf("", total_lat/CPU_CLOCK, corenum[thread_num], numanum[thread_num]);
    return NULL;
}


int main(int argc, char ** argv) {
    if (argc != 2){
        fprintf(stderr, "wrong argument\n");
    }

    for (int i=0;i<NCPU; i++) {
        corenum[i] = i;
    }

    int nthread = atoi(argv[1]);
    finished = 0;

}