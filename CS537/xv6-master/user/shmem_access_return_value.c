#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

#define PGSIZE 4096

void
test_failed()
{
	printf(1, "TEST FAILED\n");
	exit();
}

void
test_passed()
{
 printf(1, "TEST PASSED\n");
 exit();
}

int
main(int argc, char *argv[])
{
  void *ptr;

	ptr = shmem_access(2);
	if (ptr == NULL) {
        printf(1, "first: should not be null\n");
		test_failed();
	}

	if (((int) ptr) != USERTOP - PGSIZE*1) {
        printf(1, "first: should be top\n");
		test_failed();
	}

	ptr = shmem_access(3);
	if (ptr == NULL) {
        printf(1, "second: should not be null\n");
		test_failed();
	}

	if (((int) ptr) != USERTOP - PGSIZE*2) {
        printf(1, "second: should be top\n");
		test_failed();
	}

	ptr = shmem_access(0);
	if (ptr == NULL) {
        printf(1, "3rd: should not be null\n");
		test_failed();
	}

	if (((int) ptr) != USERTOP - PGSIZE*3) {
        printf(1, "3rd: should be top\n");
		test_failed();
	}

	ptr = shmem_access(1);
	if (ptr == NULL) {
        printf(1, "4-th: should not be null\n");
		test_failed();
	}

	if (((int) ptr) != USERTOP - PGSIZE*4) {
        printf(1, "4-th: should be top\n");
		test_failed();
	}
	
	test_passed();
	exit();
}
