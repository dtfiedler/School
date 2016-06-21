#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   int counter = 0;
   assert(Mem_Init(1024 * 1024) == 0);
   while(Mem_Alloc(16) != NULL)
      counter += 16;
   assert(counter >= 786432);
   exit(0);
}
