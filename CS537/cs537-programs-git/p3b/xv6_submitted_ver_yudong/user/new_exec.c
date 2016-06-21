/* Calls new_exec three times. The test passes if 3 TEST PASSED messages are seen and no TEST FAILED messages are seen */
#include "types.h"
#include "user.h"

#undef NULL
#define NULL ((void*)0)

#define PGSIZE 4096
#define PGROUNDUP(sz) (((sz)+PGSIZE-1) & ~(PGSIZE-1))

#define assert(x) if (x) {} else { \
  printf(1, "%s: %d ", __FILE__, __LINE__); \
  printf(1, "assert failed (%s)\n", # x); \
  printf(1, "TEST FAILED\n"); \
  exit(); \
}

int
main(int argc, char *argv[])
{
  //Called with 1 page for max stack
  char * args[1];
  char buf[1] = {' '};
  args[1] = buf; // Make compiler stop complaining
  int pid = fork();
  if(pid == 0) {
    new_exec("exechelper", 1, args);
    exit();
  }
  else {
	  wait();
  }
  pid = fork();
  if(pid == 0) {
    new_exec("exechelper2", 0, args);
	exit();
  }
  else {
	  wait();
  }
  pid = fork();
  if(pid == 0) {
    new_exec("exechelper3", 5, args);
	exit();
  }
  else {
	  wait();
  }
  printf(1,"TEST PASSED\n");
  exit();
}
