/* syscall argument checks (code/heap boundaries) */
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#undef NULL
#define NULL ((void*)0)

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   exit(); \
}

int
main(int argc, char *argv[])
{
  char *arg;

  // ensure stack is actually high... This is in the last user accessible page
  assert((uint) &arg > 639*1024);

  int fd = open("tmp", O_WRONLY|O_CREATE);
  assert(fd != -1);

  /* grow the heap a bit (move sz around) */
  assert((int)sbrk(4096 * 60) != -1);
  printf(1,"1\n");
  /* below code */
  arg = (char*) 0xfff;
  assert(write(fd, arg, 1) == -1);
  printf(1,"2\n");
  /* spanning code bottom */
  assert(write(fd, arg, 2) == -1);
  printf(1,"3\n");
  /* at code */
  arg = (char*) 0x1000;
  assert(write(fd, arg, 1) != -1);
  printf(1,"4\n");
  /*after code in guard*/
  arg = (char*) 0x2000;
  assert(write(fd, arg, 1) == -1);
  printf(1,"5\n");
  /*end of code*/
  arg = (char*) 0x1fff;
  assert(write(fd, arg, 1) != -1);
 printf(1,"6\n");
  /*end of code spanning guard*/
  arg = (char*) 0x1fff;
  assert(write(fd, arg, 2) == -1);
 printf(1,"7\n");
  /* above heap top */
  arg = (char*) sbrk(0);
  assert(write(fd, arg, 1) == -1);
 printf(1,"8\n");
  printf(1, "TEST PASSED\n");
  exit();
}
