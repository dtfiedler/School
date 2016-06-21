#include "types.h"
#include "user.h"

#undef NULL
#define NULL ((void*)0)

// this arv is passed to exec when this program is called the second time
char *argv[] = {"tester", "second_time", 0};

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   exit(); \
}

void worker(void *arg_ptr);

int
main(int argc, char *argv[])
{
  int i = 0;
  int pid_fork = 0;
  // second time runner sleeps for 7 seconds thus waiting for init to process the orphans
  // (sleep takes ticks as its argument, a tick is fired approximately 100 times a second)
  if (argc == 2)
    sleep(700); 
  // try to create 50 threads
  for (i = 0; i < 50; ++i) {
    assert(thread_create(worker, 0) > 0);
  }
  // if running the first time, spawn for second time with a bigger argv array
  if (argc == 1) {
    pid_fork = fork();
    argv[0] = "some value";
	argv[1] = "another value";
	argv[2] = 0;
	if (pid_fork == 0) {
      exec("tester", argv);
    }
    printf(1, "done with for loop\n");
    // first time runner exits here without joining the threads
    exit();
  }
  // second time runner, if successfully spawns 50 other threads, the test passes
  // expecting the 7 second wait to be enough for init to clean the threads spawned
  // by the first time runner
  printf(1, "TEST PASSED\n");
  exit();
}

void
worker(void *arg_ptr) {
   // don't return before your parent exits without waiting for you to die
   sleep(3);
   return;
}

