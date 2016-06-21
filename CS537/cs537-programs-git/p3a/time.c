#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mem.h"

#define MAX_POINTERS    100000
#define USABLE_SPACE    17476240
#define N               20000

int main() {
    //long start_time = time(NULL);
    struct timeval tv1;
    struct timeval tv2;
    gettimeofday(&tv1,NULL);
    unsigned long start = 1000000 * tv1.tv_sec + tv1.tv_usec;
    srand(time(NULL));
    void* p[MAX_POINTERS];
    int num_ptrs = 0;
    int total_mem = 0;
    
    // Init 50 MB
    assert(Mem_Init(50 * 1024 * 1024) == 0);
    memset(p, 0, sizeof(p));
    
    // Allocate pointers
    while (total_mem < USABLE_SPACE)
    {
        int x = 1 + rand() % 99;
        int num_bytes = x * 8;
        p[num_ptrs] = Mem_Alloc(num_bytes);
        assert(p[num_ptrs]);
        num_ptrs++;
        total_mem += num_bytes;
    }
    assert(num_ptrs >= N);
    //printf("FINISH ALLOCATE POINTER, num ptr is %d\n",num_ptrs);fflush(stdout);
    // Free N pointers
    int i;
    for (i = 0; i < N; i++)
    {
        int x = rand() % num_ptrs;
        if (!p[x])
        {
            i--;
            continue;
        }
        assert(Mem_Free(p[x]) == 0);
        p[x] = 0;
    }
    //printf("FINISH FREE N POINTER\n");fflush(stdout);
    // Try to allocate 8 byte pointers
    for (i = 0; i < N; i++)
    {
        p[num_ptrs] = Mem_Alloc(8);
        assert(p[num_ptrs]);
        num_ptrs++;
    }
    //printf("FINISH ALLOC 8B  POINTERs\n");fflush(stdout);
    // Free everything
    for (i = 0; i < num_ptrs; i++)
    {
        if (p[i])
            assert(Mem_Free(p[i]) == 0);
    }
    //printf("FINISH FREE EVERYTHING\n");fflush(stdout);
    //long end_time = time(NULL);
    gettimeofday(&tv2,NULL);
    unsigned long end  = 1000000 * tv2.tv_sec + tv2.tv_usec;
    printf("The total time of execution is (microseconds) %lu\n",end-start);
    exit(0);
}