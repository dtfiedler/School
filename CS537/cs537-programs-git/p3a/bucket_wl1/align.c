/* check first pointer returned is 8-byte aligned */
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "mem.h"

int main() {
   assert(Mem_Init(4096) == 0);
   void* ptr1 = Mem_Alloc(16);
   assert(ptr1 != NULL);
   uintptr_t addr1 = (uintptr_t)ptr1;
   //void* ptr2 = Mem_Alloc(16);
   //assert(ptr2 != NULL);
   //uintptr_t addr2 = (uintptr_t)ptr2;
   //assert((addr2-addr1) % 8 == 0);
   assert(addr1 % 8 == 0);
   exit(0);
}
