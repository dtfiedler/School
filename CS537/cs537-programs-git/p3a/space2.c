#include <assert.h>
#include <stdlib.h>
#include "mem.h"
#include <stdio.h>
int main() {
   int counter = 0;
   assert(Mem_Init(4096) == 0);
   while(Mem_Alloc(16) != NULL)
      counter += 16;
   printf("successfully alloc %d bytes\n",counter);
   assert(counter >= 983040);
   exit(0);
}
