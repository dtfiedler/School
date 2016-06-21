/* allocation is too big to fit */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   int i = 0;   
   assert(Mem_Init(4096) == 0);
   while(Mem_Alloc(16) != NULL)
      ++i;
   assert((i >= 240) && (i < 255));
   exit(0);
}
