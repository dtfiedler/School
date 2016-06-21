/* init that should be rounded up to 1 page */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   int i = 0;
   assert(Mem_Init(1) == 0);
   for (i = 0; i < 14; ++i)
   {
      assert(Mem_Alloc(256) != NULL);
   }
   assert(Mem_Alloc(80) != NULL);
   exit(0);
}
