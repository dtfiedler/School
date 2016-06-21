/* multiple threads with some depth of function calls */
#include "types.h"
#include "user.h"

#undef NULL
#define NULL ((void*)0)

#define PGSIZE (4096)

int ppid;
int global = 1;
int num_threads = 30;

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   kill(ppid); \
   exit(); \
}

void worker(void *arg_ptr);

unsigned int fib(unsigned int n) {
   if (n == 0) {
      return 0;
   } else if (n == 1) {
      return 1;
   } else {
      return fib(n - 1) + fib(n - 2);
   }
}


int
main(int argc, char *argv[])
{
   ppid = getpid();

   assert(fib(28) == 317811);
   //printf(1,"fib done\n") ;


   int arg = 101;
   void *arg_ptr = &arg;

   int i;
   for (i = 0; i < num_threads; i++) {
      int thread_pid = thread_create(worker, arg_ptr);
   //   printf(1,"done creating with the %d th thread\n",i);
      assert(thread_pid > 0);
   }

   for (i = 0; i < num_threads; i++) {
      int join_pid = join();
      assert(join_pid > 0);
   }

   printf(1, "TEST PASSED\n");
   exit();
}

void
worker(void *arg_ptr) {
   int arg = *(int*)arg_ptr;
   assert(arg == 101);
   assert(global == 1);
  // printf(1,"done with check arg and global\n");
   assert(fib(2) == 1);
   assert(fib(3) == 2);
   assert(fib(9) == 34);
   assert(fib(15) == 610);
   return;
}

