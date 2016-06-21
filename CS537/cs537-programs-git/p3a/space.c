#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mem.h"

int main() {
    srand(time(NULL));
    int total_mem = 0;
    int count = 0;
    void* ptr_array[15000];

    // Init 50 MB
    assert(Mem_Init(50 * 1024 * 1024) == 0);
    
    while(1)
    {
        int x = (rand() % 999) + 1;
        int num_bytes = x * 8;
        void* ptr = NULL;
        ptr = Mem_Alloc(num_bytes);
        if (!ptr) break;
        total_mem += num_bytes;
        ptr_array[count++] = ptr;
    }

    // Free them all
    int i;
    for (i = 0; i < count; i++)
        assert(Mem_Free(ptr_array[i]) == 0);

    printf("%d\n", total_mem);

    exit(0);
}
