#include "types.h"
#include "stat.h"
#include "user.h"

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
	
	ptr = shmem_access(-1);
	if (ptr != NULL) {
		test_failed();
	}

	ptr = shmem_access(-100);
	if (ptr != NULL) {
		test_failed();
	}

	ptr = shmem_access(4);
	if (ptr != NULL) {
		test_failed();
	}

	ptr = shmem_access(100);
	if (ptr != NULL) {
		test_failed();
	}

	test_passed();
	exit();
}
