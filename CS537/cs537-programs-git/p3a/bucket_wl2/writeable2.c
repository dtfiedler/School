/* align misaligned requests, don't overlap */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "mem.h"

int main() {
   assert(Mem_Init(4096) == 0);
   char *ptr[4];
   assert((ptr[0] = Mem_Alloc(16)) != NULL);
   assert((ptr[1] = Mem_Alloc(80)) != NULL);
   assert((ptr[2] = Mem_Alloc(256)) != NULL);
   assert((ptr[3] = Mem_Alloc(80)) != NULL);
   strcpy(ptr[0], "aaaaaaaaaaaaaaa");
   strcpy(ptr[1], "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
   strcpy(ptr[2], "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc");
   strcpy(ptr[3], "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd");
   assert(strcmp(ptr[0], "aaaaaaaaaaaaaaa") == 0);
   assert(strcmp(ptr[1], "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb") == 0);
   assert(strcmp(ptr[2], "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc") == 0);
   assert(strcmp(ptr[3], "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd") == 0);
   assert(Mem_Free(ptr[0]) == 0);
   assert(Mem_Free(ptr[1]) == 0);
   assert(Mem_Free(ptr[2]) == 0);
   assert(Mem_Free(ptr[3]) == 0);
   exit(0);
}
