/* init that should be rounded up to 1 page */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"
#include <stdio.h>

int main() {
   int i = 0;
   assert(Mem_Init(1) == 0);
   for (i = 0; i < 240; ++i)
   {
      printf("current num %d\n",i);
      assert(Mem_Alloc(16) != NULL);
   }   
   exit(0);
}
